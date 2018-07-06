#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPass.h"

#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Scenes/Headers/Scene.h"
#include "Geometry/Material/Headers/Material.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U32 g_reflectionBudget = 0;

        inline bool isInBudget() {
            return g_reflectionBudget < 
                Config::MAX_REFLECTIVE_NODES_IN_VIEW;
        }

        inline void resetBudget() {
            g_reflectionBudget = 0;
        }

        inline void updateBudget() {
            ++g_reflectionBudget;
        }
        inline U32 currentEntry() {
            return g_reflectionBudget;
        }
    };
    namespace RefractionUtil {
        U32 g_refractionBudget = 0;

        inline bool isInBudget() {
            return g_refractionBudget <
                Config::MAX_REFRACTIVE_NODES_IN_VIEW;
        }

        inline void resetBudget() {
            g_refractionBudget = 0;
        }

        inline void updateBudget() {
            ++g_refractionBudget;
        }
        inline U32 currentEntry() {
            return g_refractionBudget;
        }
    };
    
};

RenderPass::BufferData::BufferData(GFXDevice& context)
  : _lastCommandCount(0),
    _lasNodeCount(0)
{
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = Config::MAX_VISIBLE_NODES;
    bufferDescriptor._primitiveSizeInBytes = sizeof(GFXDevice::NodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._unbound = true;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;

    // This do not need to be persistently mapped as, hopefully, they will only be update once per frame
    // Each pass should have its own set of buffers (shadows, reflection, etc)
    _renderData = context.newSB(bufferDescriptor);

    bufferDescriptor._primitiveSizeInBytes = sizeof(IndirectDrawCommand);
    _cmdBuffer = context.newSB(bufferDescriptor);
    _cmdBuffer->addAtomicCounter(3);
}

RenderPass::BufferData::~BufferData()
{
}

RenderPass::BufferDataPool::BufferDataPool(GFXDevice& context, U32 maxBuffers)
    : _context(context)
{
    // Every visible node will first update this buffer with required data (WorldMatrix, NormalMatrix, Material properties, Bone count, etc)
    // Due to it's potentially huge size, it translates to (as seen by OpenGL) a Shader Storage Buffer that's persistently and coherently mapped
    // We make sure the buffer is large enough to hold data for all of our rendering stages to minimize the number of writes per frame
    // Create a shader buffer to hold all of our indirect draw commands
    // Useful if we need access to the buffer in GLSL/Compute programs
    _buffers.resize(maxBuffers, nullptr);
}

RenderPass::BufferDataPool::~BufferDataPool()
{
    _buffers.clear();
}

RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(I32 bufferIndex) {
    assert(IS_IN_RANGE_INCLUSIVE(bufferIndex, 0, to_I32(_buffers.size())));

    std::shared_ptr<BufferData>& buffer = _buffers[bufferIndex];
    // More likely case
    if (buffer) {
        return *buffer;
    }

    buffer = std::make_shared<BufferData>(_context);
    return *buffer;
}

const RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(I32 bufferIndex) const {
    assert(IS_IN_RANGE_INCLUSIVE(bufferIndex, 0, to_I32(_buffers.size())));

    const std::shared_ptr<BufferData>& buffer = _buffers[bufferIndex];
    assert(buffer != nullptr);
    return *buffer;
}

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlag)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _name(name),
      _stageFlag(passStageFlag)
{
    _lastTotalBinSize = 0;

    U32 count = getBufferCountForStage(_stageFlag);
    _passBuffers = MemoryManager_NEW BufferDataPool(_context, count);
}

RenderPass::~RenderPass() 
{
    MemoryManager::DELETE(_passBuffers);
}

RenderPass::BufferData&  RenderPass::getBufferData(I32 bufferIndex) {
    return _passBuffers->getBufferData(bufferIndex);
}

const RenderPass::BufferData&  RenderPass::getBufferData(I32 bufferIndex) const {
    return _passBuffers->getBufferData(bufferIndex);
}

void RenderPass::render(const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) {
    if (_stageFlag != RenderStage::SHADOW) {
        LightPool::bindShadowMaps(_context, bufferInOut);
    }

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            GFX::BeginDebugScopeCommand beginDebugScopeCmd;
            beginDebugScopeCmd._scopeID = 10;
            beginDebugScopeCmd._scopeName = "Display Render Stage";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            const RenderTarget& screenRT = _context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SCREEN));
            RenderPassManager::PassParams params;
            params._occlusionCull = Config::USE_HIZ_CULLING;
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());

            params._stage = _stageFlag;
            params._target = RenderTargetID(RenderTargetUsage::SCREEN);
            params._pass = 0;
            params._doPrePass = Config::USE_Z_PRE_PASS && screenRT.getAttachment(RTAttachmentType::Depth, 0).used();
            params._occlusionCull = true;
            _parent.doCustomPass(params, bufferInOut);

            GFX::EndDebugScopeCommand endDebugScopeCmd;
            GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);

            _lastTotalBinSize = _parent.getQueue().getRenderQueueStackSize();

        } break;
        case RenderStage::SHADOW: {
            GFX::BeginDebugScopeCommand beginDebugScopeCmd;
            beginDebugScopeCmd._scopeID = 20;
            beginDebugScopeCmd._scopeName = "Shadow Render Stage";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            Attorney::SceneManagerRenderPass::generateShadowMaps(_parent.parent().sceneManager(), bufferInOut);
            GFX::EndDebugScopeCommand endDebugScopeCmd;
            GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);

        } break;
        case RenderStage::REFLECTION: {
            SceneManager& mgr = _parent.parent().sceneManager();
            RenderPassManager::PassParams params;
            params._pass = Config::MAX_REFLECTIVE_NODES_IN_VIEW;
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());
            
            {
                GFX::BeginDebugScopeCommand beginDebugScopeCmd;
                beginDebugScopeCmd._scopeID = 30;
                beginDebugScopeCmd._scopeName = "Cube Reflection Render Stage";
                GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

                //Part 1 - update envirnoment maps:
                /*SceneEnvironmentProbePool* envProbPool =  Attorney::SceneRenderPass::getEnvProbes(renderState.parentScene());
                const EnvironmentProbeList& probes = envProbPool->getNearestSorted(params.camera->getEye());
                for (EnvironmentProbe_ptr& probe : probes) {
                    probe->refresh(bufferInOut);
                }
                RenderPassCuller::VisibleNodeList& mainNodeCache = mgr.getVisibleNodesCache(RenderStage::DISPLAY);
                for (const RenderPassCuller::VisibleNode& node : mainNodeCache) {
                    SceneGraphNode* nodePtr = node.second;
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    Attorney::RenderingCompRenderPass::updateEnvProbeList(*rComp, probes);
                }*/

                GFX::EndDebugScopeCommand endDebugScopeCmd;
                GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
            }
            {
                GFX::BeginDebugScopeCommand beginDebugScopeCmd;
                beginDebugScopeCmd._scopeID = 40;
                beginDebugScopeCmd._scopeName = "Planar Reflection Render Stage";
                GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

                //Part 2 - update classic reflectors (e.g. mirrors, water, etc)
                //Get list of reflective nodes from the scene manager
                const RenderPassCuller::VisibleNodeList& nodeCache = mgr.getSortedReflectiveNodes();

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                    const SceneGraphNode* nodePtr = node._node;
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    if (ReflectionUtil::isInBudget()) {
                        // Excluse node from rendering itself into the pass
                        bool isVisile = rComp->renderOptionEnabled(RenderingComponent::RenderOptions::IS_VISIBLE);
                        rComp->toggleRenderOption(RenderingComponent::RenderOptions::IS_VISIBLE, false);
                        if (Attorney::RenderingCompRenderPass::updateReflection(*rComp,
                                                                                 ReflectionUtil::currentEntry(),
                                                                                 params._camera,
                                                                                 renderState,
                                                                                 bufferInOut)) {

                            ReflectionUtil::updateBudget();
                        }
                        rComp->toggleRenderOption(RenderingComponent::RenderOptions::IS_VISIBLE, isVisile);
                    } else {
                        Attorney::RenderingCompRenderPass::clearReflection(*rComp);
                    }
                }
                GFX::EndDebugScopeCommand endDebugScopeCmd;
                GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
            }
        } break;
        case RenderStage::REFRACTION: {
            // Get list of refractive nodes from the scene manager
            SceneManager& mgr = _parent.parent().sceneManager();
            RenderPassManager::PassParams params;
            params._pass = Config::MAX_REFLECTIVE_NODES_IN_VIEW;
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());

            {
                GFX::BeginDebugScopeCommand beginDebugScopeCmd;
                beginDebugScopeCmd._scopeID = 50;
                beginDebugScopeCmd._scopeName = "Cube Refraction Render Stage";
                GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

                GFX::EndDebugScopeCommand endDebugScopeCmd;
                GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
            }
            {
                GFX::BeginDebugScopeCommand beginDebugScopeCmd;
                beginDebugScopeCmd._scopeID = 60;
                beginDebugScopeCmd._scopeName = "Planar Refraction Render Stage";
                GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

                const RenderPassCuller::VisibleNodeList& nodeCache = mgr.getSortedRefractiveNodes();
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (const RenderPassCuller::VisibleNode& node : nodeCache) {
                    const SceneGraphNode* nodePtr = node._node;
                    RenderingComponent* const rComp = nodePtr->get<RenderingComponent>();
                    if (RefractionUtil::isInBudget()) {
                        bool isVisile = rComp->renderOptionEnabled(RenderingComponent::RenderOptions::IS_VISIBLE);
                        rComp->toggleRenderOption(RenderingComponent::RenderOptions::IS_VISIBLE, false);
                        if (Attorney::RenderingCompRenderPass::updateRefraction(*rComp,
                                                                                RefractionUtil::currentEntry(),
                                                                                params._camera,
                                                                                renderState,
                                                                                bufferInOut))
                        {
                            RefractionUtil::updateBudget();
                        }
                        rComp->toggleRenderOption(RenderingComponent::RenderOptions::IS_VISIBLE, isVisile);
                    }  else {
                        Attorney::RenderingCompRenderPass::clearRefraction(*rComp);
                    }
                }

                GFX::EndDebugScopeCommand endDebugScopeCmd;
                GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);
            }
        } break;
    };
}

// This is very hackish but should hold up fine
U32 RenderPass::getBufferCountForStage(RenderStage stage) const {
    U32 maxPasses = 0;
    U32 maxStages = 0;

    //We only care about the first parameter as it will determine the properties for the rest of the stages
    switch (stage) {
        case RenderStage::REFLECTION: { //Both planar and cube
            // max reflective nodes and an extra buffer for environment maps
            maxPasses = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 2+ 1;
            maxStages = 6u; // number of cube faces
        }; break;
        case RenderStage::REFRACTION: { //Both planar and cube
            // max reflective nodes and an extra buffer for environment maps
            maxPasses = Config::MAX_REFRACTIVE_NODES_IN_VIEW * 2 + 1;
            maxStages = 1u;
        } break;
        case RenderStage::SHADOW: {
            maxPasses = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
            // Either CSM or cube map will have the highest stage count.
            maxStages = std::max(Config::Lighting::MAX_SPLITS_PER_LIGHT, 6u);// number of cube faces
        }; break;
        case RenderStage::DISPLAY: {
            maxPasses = 1;
            maxStages = 1;
        }; break;
    };


    return maxPasses * maxStages;
}

};
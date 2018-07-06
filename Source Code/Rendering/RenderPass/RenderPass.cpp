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

        inline bool isInBudget() { return g_reflectionBudget < Config::MAX_REFLECTIVE_NODES_IN_VIEW; }

        inline void resetBudget() { g_reflectionBudget = 0; }

        inline void updateBudget() { ++g_reflectionBudget; }

        inline U32 currentEntry() { return g_reflectionBudget; }
    };

    namespace RefractionUtil {
        U32 g_refractionBudget = 0;

        inline bool isInBudget() { return g_refractionBudget < Config::MAX_REFRACTIVE_NODES_IN_VIEW;  }

        inline void resetBudget() { g_refractionBudget = 0; }

        inline void updateBudget() { ++g_refractionBudget;  }

        inline U32 currentEntry() { return g_refractionBudget; }
    };
    
};

RenderPass::BufferData::BufferData(GFXDevice& context, U32 sizeFactor, I32 index)
  : _sizeFactor(sizeFactor),
    _lastCommandCount(0)
{
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._primitiveCount = Config::MAX_VISIBLE_NODES * _sizeFactor;
    bufferDescriptor._primitiveSizeInBytes = sizeof(GFXDevice::NodeData);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) | to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
    bufferDescriptor._name = Util::StringFormat("RENDER_DATA_%d", index).c_str();
    // This do not need to be persistently mapped as, hopefully, they will only be update once per frame
    // Each pass should have its own set of buffers (shadows, reflection, etc)
    _renderData = context.newSB(bufferDescriptor);

    bufferDescriptor._primitiveCount = Config::MAX_VISIBLE_NODES;
    bufferDescriptor._primitiveSizeInBytes = sizeof(IndirectDrawCommand);
    _cmdBuffers.reserve(_sizeFactor);

    for (U32 i = 0; i < _sizeFactor; ++i) {
        bufferDescriptor._name = Util::StringFormat("CMD_DATA_%d_%d", index, i).c_str();
        _cmdBuffers.push_back(context.newSB(bufferDescriptor));
        _cmdBuffers.back()->addAtomicCounter(1, 5);
    }
}

RenderPass::BufferData::~BufferData()
{
}

RenderPass::BufferDataPool::BufferDataPool(GFXDevice& context, const BufferDataPoolParams& params)
    : _context(context),
      _bufferSizeFactor(params._maxPassesPerBuffer)
{
    _buffers.resize(params._maxBuffers, nullptr);
}

RenderPass::BufferDataPool::~BufferDataPool()
{
    _buffers.clear();
}

RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(I32 bufferIndex, I32 bufferOffset) {
    std::shared_ptr<BufferData>& bufferData = _buffers[bufferIndex];
    bufferData->_renderDataElementOffset = Config::MAX_VISIBLE_NODES * bufferOffset;
    return *bufferData;
}

const RenderPass::BufferData& RenderPass::BufferDataPool::getBufferData(I32 bufferIndex, I32 bufferOffset) const {
    const std::shared_ptr<BufferData>& bufferData = _buffers[bufferIndex];
    bufferData->_renderDataElementOffset = Config::MAX_VISIBLE_NODES * bufferOffset;

    return *bufferData;
}

void RenderPass::BufferDataPool::initBuffers() {
    I32 i = 0;
    for (std::shared_ptr<BufferData>& buffer : _buffers) {
        buffer = std::make_shared<BufferData>(_context, _bufferSizeFactor, i++);
    }
}

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlag)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _name(name),
      _stageFlag(passStageFlag)
{
    _lastTotalBinSize = 0;

    BufferDataPoolParams params = getBufferParamsForStage(_stageFlag);
    _passBuffers = MemoryManager_NEW BufferDataPool(_context, params);
}

RenderPass::~RenderPass() 
{
    MemoryManager::DELETE(_passBuffers);
}

RenderPass::BufferData&  RenderPass::getBufferData(I32 bufferIndex, I32 bufferOffset) {
    return _passBuffers->getBufferData(bufferIndex, bufferOffset);
}

const RenderPass::BufferData&  RenderPass::getBufferData(I32 bufferIndex, I32 bufferOffset) const {
    return _passBuffers->getBufferData(bufferIndex, bufferOffset);
}

void RenderPass::initBufferData() {
    _passBuffers->initBuffers();
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
            params._doPrePass = screenRT.getAttachment(RTAttachmentType::Depth, 0).used();
            _parent.doCustomPass(params, bufferInOut);

            GFX::EndDebugScopeCommand endDebugScopeCmd;
            GFX::EnqueueCommand(bufferInOut, endDebugScopeCmd);

            _lastTotalBinSize = _parent.getQueue().getRenderQueueStackSize(_stageFlag);

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
                RenderPassCuller::VisibleNodeList nodes = mgr.getSortedReflectiveNodes(*params._camera, RenderStage::REFLECTION, true);

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (RenderPassCuller::VisibleNode& node : nodes) {
                    RenderingComponent* const rComp = node._node->get<RenderingComponent>();
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

                RenderPassCuller::VisibleNodeList nodes = mgr.getSortedRefractiveNodes(*params._camera, RenderStage::REFRACTION, true);
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (RenderPassCuller::VisibleNode& node : nodes) {
                    RenderingComponent* const rComp = node._node->get<RenderingComponent>();
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
RenderPass::BufferDataPoolParams RenderPass::getBufferParamsForStage(RenderStage stage) const {

    RenderPass::BufferDataPoolParams ret;
    ret._maxPassesPerBuffer = 6;
    //We only care about the first parameter as it will determine the properties for the rest of the stages
    switch (stage) {
        case RenderStage::REFLECTION: { //Both planar and cube
            // max reflective nodes and an extra buffer for environment maps
            ret._maxBuffers = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 2 + 1;
        }; break;
        case RenderStage::REFRACTION: { //Both planar and cube
            ret._maxBuffers = Config::MAX_REFRACTIVE_NODES_IN_VIEW * 2 + 1;
        } break;
        case RenderStage::SHADOW: {
            ret._maxBuffers = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS;
        }; break;
        case RenderStage::DISPLAY: {
            ret._maxBuffers = 1;
            ret._maxPassesPerBuffer = 1;
        }; break;
    };

    // We might need new buffer data for each pass type (prepass, main, oit, etc)
    ret._maxPassesPerBuffer *= to_base(RenderPassType::COUNT);

    return ret;
}

};
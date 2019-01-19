#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPass.h"

#include "Core/Headers/Kernel.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Scenes/Headers/Scene.h"
#include "Geometry/Material/Headers/Material.h"

#include "ECS/Components/Headers/RenderingComponent.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    // How many cmd buffers should we create (as a factor) so that we can swap them between frames
    constexpr U32 g_cmdBufferFrameCount = 3;

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

    // This is very hackish but should hold up fine
    U32 getDataBufferSize(RenderStage stage) {
        U32 bufferSizeFactor = 1;
        //We only care about the first parameter as it will determine the properties for the rest of the stages
        switch (stage) {
            // PrePass, MainPass and OitPass should share buffers
            case RenderStage::DISPLAY: break;
            // Planar, cube & environment
            case RenderStage::REFRACTION:
            case RenderStage::REFLECTION: {
                bufferSizeFactor = Config::MAX_REFLECTIVE_NODES_IN_VIEW;
                bufferSizeFactor *= 2;
                bufferSizeFactor += 1;
            }; break;
            case RenderStage::SHADOW: {
                // One buffer per light, but split into multiple pieces
                bufferSizeFactor = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS * 6;
            }; break;
        };

        return bufferSizeFactor * Config::MAX_VISIBLE_NODES;
    }

    U32 getBufferOffset(RenderStage stage, RenderPassType type, U32 passIndex) {
        U32 ret = 0;

        switch (stage) {
            case RenderStage::DISPLAY: break;
            case RenderStage::REFLECTION:
            case RenderStage::REFRACTION: {
                switch (passIndex) {
                    case 0: ret = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 0; break; // planar
                    case 1: ret = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 1; break; // cube
                    case 2: ret = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 2; break; // environment
                    default: assert(false && "getBufferOffset error: invalid pass index"); break;
                };
            }break;
            case RenderStage::SHADOW: {
                ret = Config::MAX_VISIBLE_NODES * passIndex;
            }break;

        }
        return ret;
    }

    U32 getCmdBufferCount(RenderStage stage) {
        U32 ret = 0;

        switch (stage) {
            case RenderStage::REFLECTION: // planar, cube & environment
            case RenderStage::REFRACTION: ret = 3; break;
            case RenderStage::DISPLAY: ret = 1;  break;
            case RenderStage::SHADOW: ret = Config::Lighting::MAX_SHADOW_CASTING_LIGHTS * 6;  break;
        };

        return ret;
    }

    U32 getCmdBufferIndex(RenderStage stage, I32 passIndex) {
        U32 ret = 0;
        if (stage == RenderStage::SHADOW) {
            ret = passIndex;
        }
        return ret;
    }
};

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlag)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _name(name),
      _stageFlag(passStageFlag)
{
    _lastTotalBinSize = 0;
    _dataBufferSize = getDataBufferSize(_stageFlag);
}

RenderPass::~RenderPass() 
{
}

RenderPass::BufferData RenderPass::getBufferData(RenderPassType type, I32 passIndex) const {
    U32 idx = getCmdBufferIndex(_stageFlag, passIndex);
    idx += getCmdBufferCount(_stageFlag) * (_context.FRAME_COUNT % g_cmdBufferFrameCount);

    BufferData ret = {};
    ret._renderDataElementOffset = getBufferOffset(_stageFlag, type, passIndex);
    ret._renderData = _renderData;
    ret._cmdBuffer = _cmdBuffers[idx].first;
    ret._lastCommandCount = &_cmdBuffers[idx].second;

    return ret;
}

void RenderPass::initBufferData() {
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._elementCount = _dataBufferSize;
    bufferDescriptor._elementSize = sizeof(GFXDevice::NodeData);

    bufferDescriptor._ringBufferLength = g_cmdBufferFrameCount;
    bufferDescriptor._separateReadWrite = false;
    
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::UNBOUND_STORAGE) | to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._name = Util::StringFormat("RENDER_DATA_%s", TypeUtil::renderStageToString(_stageFlag)).c_str();
    _renderData = _context.newSB(bufferDescriptor);

    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._elementCount = Config::MAX_VISIBLE_NODES;
    bufferDescriptor._elementSize = sizeof(IndirectDrawCommand);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._separateReadWrite = false;

    U32 cmdCount = getCmdBufferCount(_stageFlag) * g_cmdBufferFrameCount;
    _cmdBuffers.reserve(cmdCount);

    for (U32 i = 0; i < cmdCount; ++i) {
        bufferDescriptor._name = Util::StringFormat("CMD_DATA_%s_%d", TypeUtil::renderStageToString(_stageFlag), i).c_str();
        _cmdBuffers.push_back(std::make_pair(_context.newSB(bufferDescriptor), 0));
        _cmdBuffers.back().first->addAtomicCounter(1, 5);
    }
}

void RenderPass::render(const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) {
    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            GFX::BeginDebugScopeCommand beginDebugScopeCmd;
            beginDebugScopeCmd._scopeID = 10;
            beginDebugScopeCmd._scopeName = "Display Render Stage";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            RenderPassManager::PassParams params = {};
            params._occlusionCull = Config::USE_HIZ_CULLING;
            params._stage = _stageFlag;
            params._target = RenderTargetID(RenderTargetUsage::SCREEN);
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());

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
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());

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
                RenderPassCuller::VisibleNodeList nodes = mgr.getSortedReflectiveNodes(*camera, RenderStage::REFLECTION, true);

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (RenderPassCuller::VisibleNode& node : nodes) {
                    RenderingComponent* const rComp = node._node->get<RenderingComponent>();
                    if (ReflectionUtil::isInBudget()) {
                        if (Attorney::RenderingCompRenderPass::updateReflection(*rComp,
                                                                                 ReflectionUtil::currentEntry(),
                                                                                 camera,
                                                                                 renderState,
                                                                                 bufferInOut)) {

                            ReflectionUtil::updateBudget();
                        }
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
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());
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

                RenderPassCuller::VisibleNodeList nodes = mgr.getSortedRefractiveNodes(*camera, RenderStage::REFRACTION, true);
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (RenderPassCuller::VisibleNode& node : nodes) {
                     RenderingComponent* const rComp = node._node->get<RenderingComponent>();
                     if (RefractionUtil::isInBudget()) {
                        if (Attorney::RenderingCompRenderPass::updateRefraction(*rComp,
                                                                                RefractionUtil::currentEntry(),
                                                                                camera,
                                                                                renderState,
                                                                                bufferInOut))
                        {
                            RefractionUtil::updateBudget();
                        }
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

void RenderPass::postRender() {
    _renderData->incQueue();
}

};
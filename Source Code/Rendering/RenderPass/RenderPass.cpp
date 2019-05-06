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
                bufferSizeFactor = ShadowMap::MAX_SHADOW_PASSES;
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
            case RenderStage::SHADOW: ret = ShadowMap::MAX_SHADOW_PASSES;  break;
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

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, stringImpl name, U8 sortKey, RenderStage passStageFlag, const vector<U8>& dependencies, bool performanceCounters)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _dependencies(dependencies),
      _name(name),
      _stageFlag(passStageFlag),
      _performanceCounters(performanceCounters)
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
	ret._cullCounter = _cullCounter;
    ret._cmdBuffer = _cmdBuffers[idx].first;
    ret._lastCommandCount = &_cmdBuffers[idx].second;

    return ret;
}

void RenderPass::initBufferData() {
    ShaderBufferDescriptor bufferDescriptor;
    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._elementCount = _dataBufferSize;
    bufferDescriptor._elementSize = sizeof(GFXDevice::NodeData);

    bufferDescriptor._ringBufferLength = g_cmdBufferFrameCount;
    bufferDescriptor._separateReadWrite = false;
    
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._name = Util::StringFormat("RENDER_DATA_%s", TypeUtil::renderStageToString(_stageFlag)).c_str();
    _renderData = _context.newSB(bufferDescriptor);

    if (_performanceCounters) {
        bufferDescriptor._usage = ShaderBuffer::Usage::ATOMIC_COUNTER;
        bufferDescriptor._name = Util::StringFormat("CULL_COUNTER_%s", TypeUtil::renderStageToString(_stageFlag)).c_str();
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
        bufferDescriptor._elementCount = 1;
        bufferDescriptor._elementSize = sizeof(U32);
        bufferDescriptor._ringBufferLength = 5;
        bufferDescriptor._separateReadWrite = true;
        _cullCounter = _context.newSB(bufferDescriptor);
    }

    bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
    bufferDescriptor._elementCount = Config::MAX_VISIBLE_NODES;
    bufferDescriptor._elementSize = sizeof(IndirectDrawCommand);
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._separateReadWrite = false;

    U32 cmdCount = getCmdBufferCount(_stageFlag) * g_cmdBufferFrameCount;
    _cmdBuffers.reserve(cmdCount);

    for (U32 i = 0; i < cmdCount; ++i) {
        bufferDescriptor._name = Util::StringFormat("CMD_DATA_%s_%d", TypeUtil::renderStageToString(_stageFlag), i).c_str();
        _cmdBuffers.emplace_back(std::make_pair(_context.newSB(bufferDescriptor), 0));
    }
}

void RenderPass::render(const Task& parentTask, const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) {
    ACKNOWLEDGE_UNUSED(parentTask);

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            RenderPassManager::PassParams params = {};
            params._stage = _stageFlag;
            params._target = RenderTargetID(RenderTargetUsage::SCREEN);
            params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z);
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());

            GFX::CopyTextureCommand copyCmd = {};
            copyCmd._source = _context.renderTargetPool().renderTarget(params._targetHIZ).getAttachment(RTAttachmentType::Depth, 0).texture();
            copyCmd._destination = _context.getPrevDepthBuffer();
            GFX::EnqueueCommand(bufferInOut, copyCmd);

            _parent.doCustomPass(params, bufferInOut);

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
            }
            {
                //Part 2 - update classic reflectors (e.g. mirrors, water, etc)
                //Get list of reflective nodes from the scene manager
                VisibleNodeList nodes = mgr.getSortedReflectiveNodes(*camera, RenderStage::REFLECTION, true);

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (VisibleNode& node : nodes) {
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
            }
        } break;
        case RenderStage::REFRACTION: {
            // Get list of refractive nodes from the scene manager
            SceneManager& mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());
            {
            }
            {
                VisibleNodeList nodes = mgr.getSortedRefractiveNodes(*camera, RenderStage::REFRACTION, true);
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (VisibleNode& node : nodes) {
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
            }
        } break;
    };
}

void RenderPass::postRender() {
    _renderData->incQueue();
}

};
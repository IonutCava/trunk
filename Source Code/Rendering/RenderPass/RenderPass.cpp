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
    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U16 g_reflectionBudget = 0;

        inline bool isInBudget() noexcept { return g_reflectionBudget < Config::MAX_REFLECTIVE_NODES_IN_VIEW; }
        inline void resetBudget() noexcept { g_reflectionBudget = 0; }
        inline void updateBudget() noexcept { ++g_reflectionBudget; }
        inline U16  currentEntry() noexcept { return g_reflectionBudget; }
    };

    namespace RefractionUtil {
        U16 g_refractionBudget = 0;

        inline bool isInBudget() noexcept { return g_refractionBudget < Config::MAX_REFRACTIVE_NODES_IN_VIEW;  }
        inline void resetBudget() noexcept { g_refractionBudget = 0; }
        inline void updateBudget() noexcept { ++g_refractionBudget;  }
        inline U16  currentEntry() noexcept { return g_refractionBudget; }
    };

    U32 getBufferFactor(RenderStage stage) noexcept {
        //We only care about the first parameter as it will determine the properties for the rest of the stages
        switch (stage) {
            // PrePass, MainPass and OitPass should share buffers
            case RenderStage::DISPLAY:    return 1u;
            case RenderStage::SHADOW:     return ShadowMap::MAX_SHADOW_PASSES;
            case RenderStage::REFRACTION: return Config::MAX_REFRACTIVE_NODES_IN_VIEW; // planar
            case RenderStage::REFLECTION: return Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + // could be planar
                                                 Config::MAX_REFLECTIVE_PROBES_PER_PASS * 6; // environment
        };

        DIVIDE_UNEXPECTED_CALL();
        return 0u;
    }
};

U8 RenderPass::DataBufferRingSize() {
    // Size factor for command and data bufeers
    constexpr U8 g_bufferSizeFactor = 3;

    return g_bufferSizeFactor;
}

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, Str64 name, U8 sortKey, RenderStage passStageFlag, const vectorSTD<U8>& dependencies, bool performanceCounters)
    : _parent(parent),
      _context(context),
      _sortKey(sortKey),
      _dependencies(dependencies),
      _name(name),
      _stageFlag(passStageFlag),
      _performanceCounters(performanceCounters),
      _lastCmdCount(0u),
      _lastNodeCount(0u)
{
}

void RenderPass::initBufferData() {
    {// Atomic counter for occlusion culling
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::ATOMIC_COUNTER;
        bufferDescriptor._elementCount = 1;
        bufferDescriptor._elementSize = sizeof(U32);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OCASSIONAL;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = 5;
        bufferDescriptor._separateReadWrite = true;

        if (_performanceCounters) {
            bufferDescriptor._name = Util::StringFormat("CULL_COUNTER_%s", TypeUtil::RenderStageToString(_stageFlag)).c_str();
            _cullCounter = _context.newSB(bufferDescriptor);
        }
    }
    {// Node Data buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_VISIBLE_NODES;
        bufferDescriptor._elementSize = sizeof(GFXDevice::NodeData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize();
        bufferDescriptor._separateReadWrite = false;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        { 
            bufferDescriptor._name = Util::StringFormat("RENDER_DATA_%s", TypeUtil::RenderStageToString(_stageFlag)).c_str();
            _nodeData = _context.newSB(bufferDescriptor);
        }
    }
    {// Indirect draw command buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_VISIBLE_NODES;
        bufferDescriptor._elementSize = sizeof(IndirectDrawCommand);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize();
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        bufferDescriptor._separateReadWrite = false;
        {
            bufferDescriptor._name = Util::StringFormat("CMD_DATA_%s", TypeUtil::RenderStageToString(_stageFlag)).c_str();
            _cmdBuffer = _context.newSB(bufferDescriptor);
        }
    }
}

RenderPass::BufferData RenderPass::getBufferData(const RenderStagePass& stagePass) const {
    assert(_stageFlag == stagePass._stage);

    U32 cmdBufferIdx = 0u;
    switch (_stageFlag) {
        case RenderStage::DISPLAY:    cmdBufferIdx = 0u; break;
        case RenderStage::SHADOW:     cmdBufferIdx = stagePass._indexA * ShadowMap::MAX_PASSES_PER_LIGHT + stagePass._indexB; break;
        case RenderStage::REFRACTION: {
            assert(stagePass._variant == to_base(RefractorType::PLANAR));
            cmdBufferIdx = stagePass._indexA * Config::MAX_REFRACTIVE_NODES_IN_VIEW + stagePass._indexB;
        } break;
        case RenderStage::REFLECTION: {
            switch (static_cast<ReflectorType>(stagePass._variant)) {
                case ReflectorType::PLANAR: 
                case ReflectorType::CUBE:
                    cmdBufferIdx = stagePass._indexA * 6 + stagePass._indexB;
                    break;
                case ReflectorType::ENVIRONMENT:
                    cmdBufferIdx = Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + stagePass._indexA * 6 + stagePass._indexB;
                    break;
                default: DIVIDE_UNEXPECTED_CALL(); 
                    break;
            };
        } break;
        default: DIVIDE_UNEXPECTED_CALL(); break;
    }

    BufferData ret = {};
    ret._cullCounter = _cullCounter;

    ret._nodeData = _nodeData;
    ret._cmdBuffer = _cmdBuffer;
    ret._elementOffset = cmdBufferIdx * Config::MAX_VISIBLE_NODES;

    ret._lastCommandCount = &_lastCmdCount;
    ret._lastNodeCount = &_lastNodeCount;
    return ret;
}

void RenderPass::render(const Task& parentTask, const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) {
    OPTICK_EVENT();

    ACKNOWLEDGE_UNUSED(parentTask);

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            OPTICK_EVENT("RenderPass - Main");

            RTClearDescriptor clearDescriptor = {};
            clearDescriptor.clearColours(true);
            clearDescriptor.clearDepth(true);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::ALBEDO), false);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::EXTRA), true);

            RenderPassManager::PassParams params = {};
            params._stagePass = { _stageFlag, RenderPassType::COUNT };
            params._target = _context.renderTargetPool().screenTargetID();
            params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z);
            params._targetOIT = params._target._usage == RenderTargetUsage::SCREEN_MS ? RenderTargetID(RenderTargetUsage::OIT_MS) : RenderTargetID(RenderTargetUsage::OIT);
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(*_parent.parent().sceneManager());
            params._clearDescriptor = &clearDescriptor;
            params._passName = "MainRenderPass";
            _parent.doCustomPass(params, bufferInOut);
        } break;
        case RenderStage::SHADOW: {
            OPTICK_EVENT("RenderPass - Shadow");
            GFX::BeginDebugScopeCommand beginDebugScopeCmd = {};
            beginDebugScopeCmd._scopeID = 20;
            beginDebugScopeCmd._scopeName = "Shadow Render Stage";
            GFX::EnqueueCommand(bufferInOut, beginDebugScopeCmd);

            GFX::SetClippingStateCommand clipStateCmd = {};
            clipStateCmd._negativeOneToOneDepth = true;
            GFX::EnqueueCommand(bufferInOut, clipStateCmd);

            Attorney::SceneManagerRenderPass::generateShadowMaps(*_parent.parent().sceneManager(), bufferInOut);

            clipStateCmd._negativeOneToOneDepth = false;
            GFX::EnqueueCommand(bufferInOut, clipStateCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;
        case RenderStage::REFLECTION: {
            OPTICK_EVENT("RenderPass - Reflection");
            SceneManager* mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(*mgr);

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
                const VisibleNodeList& nodes = mgr->getSortedReflectiveNodes(*camera, RenderStage::REFLECTION, true);

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (const VisibleNode& node : nodes) {
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
            OPTICK_EVENT("RenderPass - Refraction");
            // Get list of refractive nodes from the scene manager
            SceneManager* mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(*mgr);
            {
                const VisibleNodeList& nodes = mgr->getSortedRefractiveNodes(*camera, RenderStage::REFRACTION, true);
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (const VisibleNode& node : nodes) {
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
    OPTICK_EVENT();

    _nodeData->incQueue();
    _cmdBuffer->incQueue();
}

};
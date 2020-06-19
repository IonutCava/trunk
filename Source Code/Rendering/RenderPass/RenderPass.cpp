#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPass.h"

#include "Core/Headers/Kernel.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Managers/Headers/SceneManager.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Geometry/Material/Headers/Material.h"
#include "Scenes/Headers/Scene.h"

#include "ECS/Components/Headers/EnvironmentProbeComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U16 g_reflectionBudget = 0;

        bool isInBudget() noexcept { return g_reflectionBudget < Config::MAX_REFLECTIVE_NODES_IN_VIEW; }
        void resetBudget() noexcept { g_reflectionBudget = 0; }
        void updateBudget() noexcept { ++g_reflectionBudget; }
        U16  currentEntry() noexcept { return g_reflectionBudget; }
    };

    namespace RefractionUtil {
        U16 g_refractionBudget = 0;

        bool isInBudget() noexcept { return g_refractionBudget < Config::MAX_REFRACTIVE_NODES_IN_VIEW;  }
        void resetBudget() noexcept { g_refractionBudget = 0; }
        void updateBudget() noexcept { ++g_refractionBudget;  }
        U16  currentEntry() noexcept { return g_refractionBudget; }
    };

    U32 getBufferFactor(RenderStage stage) noexcept {
        //We only care about the first parameter as it will determine the properties for the rest of the stages
        switch (stage) {
            // PrePass, MainPass and OitPass should share buffers
            case RenderStage::DISPLAY:    return 1u;
            case RenderStage::SHADOW:     return Config::Lighting::MAX_SHADOW_PASSES;
            case RenderStage::REFRACTION: return Config::MAX_REFRACTIVE_NODES_IN_VIEW; // planar
            case RenderStage::REFLECTION: return Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + // could be planar
                                                 Config::MAX_REFLECTIVE_PROBES_PER_PASS * 6; // environment
            default: break;
        };

        DIVIDE_UNEXPECTED_CALL();
        return 0u;
    }
};

U8 RenderPass::DataBufferRingSize() {
    // Size factor for command and data buffers
    constexpr U8 g_bufferSizeFactor = 3;

    return g_bufferSizeFactor;
}

RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, Str64 name, U8 sortKey, RenderStage passStageFlag, const vectorEASTL<U8>& dependencies, bool performanceCounters)
    : _context(context),
      _parent(parent),
      _sortKey(sortKey),
      _dependencies(dependencies),
      _name(name),
      _stageFlag(passStageFlag),
      _performanceCounters(performanceCounters)
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
            bufferDescriptor._name = Util::StringFormat("NODE_DATA_%s", TypeUtil::RenderStageToString(_stageFlag)).c_str();
            _nodeData = _context.newSB(bufferDescriptor);
        }
    }
    {// Collision Data buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_VISIBLE_NODES;
        bufferDescriptor._elementSize = sizeof(GFXDevice::CollisionData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize();
        bufferDescriptor._separateReadWrite = false;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        {
            bufferDescriptor._name = Util::StringFormat("COLLISION_DATA_%s", TypeUtil::RenderStageToString(_stageFlag)).c_str();
            _colData = _context.newSB(bufferDescriptor);
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
        case RenderStage::DISPLAY:    
        case RenderStage::SHADOW: cmdBufferIdx = RenderStagePass::indexForStage(stagePass); break;
        case RenderStage::REFRACTION: { 
            switch (static_cast<RefractorType>(stagePass._variant)) {
                case RefractorType::PLANAR:
                    cmdBufferIdx = stagePass._index;
                    break;
                default: DIVIDE_UNEXPECTED_CALL();
                    break;
            };
        } break;
        case RenderStage::REFLECTION: {
            switch (static_cast<ReflectorType>(stagePass._variant)) {
                case ReflectorType::PLANAR:  //We buffer for worst case scenario anyway (6 passes)
                case ReflectorType::CUBE:
                    cmdBufferIdx = stagePass._index * 6 + stagePass._pass;
                    break;
                default: DIVIDE_UNEXPECTED_CALL(); 
                    break;
            };
        } break;
        default: DIVIDE_UNEXPECTED_CALL(); break;
    }

    BufferData ret;
    ret._cullCounter = _cullCounter;
    ret._nodeData = _nodeData;
    ret._colData = _colData;
    ret._cmdBuffer = _cmdBuffer;
    ret._elementOffset = cmdBufferIdx * Config::MAX_VISIBLE_NODES;
    ret._lastCommandCount = &_lastCmdCount;
    ret._lastNodeCount = &_lastNodeCount;
    return ret;
}

void RenderPass::render(const Task& parentTask, const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) const {
    OPTICK_EVENT();

    ACKNOWLEDGE_UNUSED(parentTask);

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            OPTICK_EVENT("RenderPass - Main");
            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Main Display Pass" });

            RTClearDescriptor clearDescriptor = {};
            clearDescriptor.clearColours(true);
            clearDescriptor.clearDepth(true);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::ALBEDO), false);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);
            clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::EXTRA), true);

            RTDrawDescriptor normalsAndDepthPolicy = {};
            normalsAndDepthPolicy.drawMask().disableAll();
            normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);
            normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA), true);
            normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);

            RenderPassParams params = {};
            params._stagePass = RenderStagePass{ _stageFlag, RenderPassType::COUNT };
            params._target = _context.renderTargetPool().screenTargetID();
            params._targetDescriptorPrePass = normalsAndDepthPolicy;

            RTDrawDescriptor mainPassPolicy = {};
            mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
            mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), false);
            mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA), false);
            params._targetDescriptorMainPass = mainPassPolicy;

            params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z);
            params._targetOIT = params._target._usage == RenderTargetUsage::SCREEN_MS ? RenderTargetID(RenderTargetUsage::OIT_MS) : RenderTargetID(RenderTargetUsage::OIT);
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());
            params._passName = "MainRenderPass";

            GFX::ClearRenderTargetCommand clearMainTarget = {};
            clearMainTarget._target = params._target;
            clearMainTarget._descriptor = clearDescriptor;
            GFX::EnqueueCommand(bufferInOut, clearMainTarget);

            _parent.doCustomPass(params, bufferInOut);

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
        } break;
        case RenderStage::SHADOW: {
            OPTICK_EVENT("RenderPass - Shadow");
            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Shadow Render Stage" });

            //ToDo: remove this and change lookup code
            GFX::SetClippingStateCommand clipStateCmd;
            clipStateCmd._negativeOneToOneDepth = true;
            //GFX::EnqueueCommand(bufferInOut, clipStateCmd);

            Attorney::SceneManagerRenderPass::generateShadowMaps(_parent.parent().sceneManager(), bufferInOut);

            clipStateCmd._negativeOneToOneDepth = false;
            //GFX::EnqueueCommand(bufferInOut, clipStateCmd);

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;
        case RenderStage::REFLECTION: {
            static VisibleNodeList s_Nodes;

            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Reflection Pass" });

            SceneManager* mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(mgr);
            SceneEnvironmentProbePool* envProbPool = Attorney::SceneRenderPass::getEnvProbes(mgr->getActiveScene());
            {
                envProbPool->prepare(bufferInOut);
                OPTICK_EVENT("RenderPass - Probes");
                envProbPool->lockProbeList();
                const EnvironmentProbeList& probes = envProbPool->sortAndGetLocked(camera->getEye());
                U32 idx = 0u;
                for (const auto& probe : probes) {
                    probe->refresh(bufferInOut);
                    if (++idx == Config::MAX_REFLECTIVE_PROBES_PER_PASS) {
                        break;
                    }
                }
                envProbPool->unlockProbeList();
            }
            OPTICK_EVENT("RenderPass - Reflection");
            {
                //Update classic reflectors (e.g. mirrors, water, etc)
                //Get list of reflective nodes from the scene manager
                mgr->getSortedReflectiveNodes(camera, RenderStage::REFLECTION, true, s_Nodes);

                // While in budget, update reflections
                ReflectionUtil::resetBudget();
                for (size_t i = 0; i < s_Nodes.size(); ++i) {
                    const VisibleNode& node = s_Nodes.node(i);
                    RenderingComponent* const rComp = node._node->get<RenderingComponent>();
                    if (Attorney::RenderingCompRenderPass::updateReflection(*rComp,
                                                                            ReflectionUtil::currentEntry(),
                                                                            ReflectionUtil::isInBudget(),
                                                                            camera,
                                                                            renderState,
                                                                            bufferInOut)) {

                        ReflectionUtil::updateBudget();
                    }
                }
            }
            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;
        case RenderStage::REFRACTION: {
            static VisibleNodeList s_Nodes;

            GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Refraction Pass" });

            OPTICK_EVENT("RenderPass - Refraction");
            // Get list of refractive nodes from the scene manager
            SceneManager* mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(mgr);
            {
                mgr->getSortedRefractiveNodes(camera, RenderStage::REFRACTION, true, s_Nodes);
                // While in budget, update refractions
                RefractionUtil::resetBudget();
                for (size_t i = 0; i < s_Nodes.size(); ++i) {
                     const VisibleNode& node = s_Nodes.node(i);
                     RenderingComponent* const rComp = node._node->get<RenderingComponent>();
                     if (Attorney::RenderingCompRenderPass::updateRefraction(*rComp,
                                                                            RefractionUtil::currentEntry(),
                                                                            RefractionUtil::isInBudget(),
                                                                            camera,
                                                                            renderState,
                                                                            bufferInOut))
                    {
                        RefractionUtil::updateBudget();
                    }
                }
            }

            GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;
        default: DIVIDE_UNEXPECTED_CALL(); break;
    };
}

void RenderPass::postRender() const {
    OPTICK_EVENT();

    _nodeData->incQueue();
    _colData->incQueue();
    _cmdBuffer->incQueue();
}

};
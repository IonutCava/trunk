#include "stdafx.h"

#include "config.h"

#include "Headers/RenderPass.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Managers/Headers/SceneManager.h"

#include "Platform/Video/Headers/GFXDevice.h"

#include "Geometry/Material/Headers/Material.h"
#include "Scenes/Headers/Scene.h"

#include "ECS/Components/Headers/EnvironmentProbeComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "Headers/NodeBufferedData.h"

#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace {
    // We need a proper, time-based system, to check reflection budget
    namespace ReflectionUtil {
        U16 g_reflectionBudget = 0;

        [[nodiscard]] bool isInBudget() noexcept { return g_reflectionBudget < Config::MAX_REFLECTIVE_NODES_IN_VIEW; }
                      void resetBudget() noexcept { g_reflectionBudget = 0; }
                      void updateBudget() noexcept { ++g_reflectionBudget; }
        [[nodiscard]] U16  currentEntry() noexcept { return g_reflectionBudget; }
    };

    namespace RefractionUtil {
        U16 g_refractionBudget = 0;

        [[nodiscard]] bool isInBudget() noexcept { return g_refractionBudget < Config::MAX_REFRACTIVE_NODES_IN_VIEW;  }
                      void resetBudget() noexcept { g_refractionBudget = 0; }
                      void updateBudget() noexcept { ++g_refractionBudget;  }
        [[nodiscard]] U16  currentEntry() noexcept { return g_refractionBudget; }
    };

    [[nodiscard]] U32 getBufferFactor(const RenderStage stage) noexcept {
        //We only care about the first parameter as it will determine the properties for the rest of the stages
        switch (stage) {
            // PrePass, MainPass and OitPass should share buffers
            case RenderStage::DISPLAY:    return 1u;
            case RenderStage::SHADOW:     return Config::Lighting::MAX_SHADOW_PASSES;
            case RenderStage::REFRACTION: return Config::MAX_REFRACTIVE_NODES_IN_VIEW; // planar
            case RenderStage::REFLECTION: return Config::MAX_REFLECTIVE_NODES_IN_VIEW * 6 + // could be planar
                                                 Config::MAX_REFLECTIVE_PROBES_PER_PASS * 6; // environment
            case RenderStage::COUNT:      break;
        };

        DIVIDE_UNEXPECTED_CALL();
        return 0u;
    }
};


RenderPass::RenderPass(RenderPassManager& parent, GFXDevice& context, Str64 name, const U8 sortKey, const RenderStage passStageFlag, const vectorEASTL<U8>& dependencies, const bool performanceCounters)
    : _context(context),
      _parent(parent),
      _config(context.context().config()),
      _sortKey(sortKey),
      _dependencies(dependencies),
      _name(MOV(name)),
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
            bufferDescriptor._name = Util::StringFormat("CULL_COUNTER_%s", TypeUtil::RenderStageToString(_stageFlag));
            _cullCounter = _context.newSB(bufferDescriptor);
        }
    }
    {// Node Transform buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_VISIBLE_NODES;
        bufferDescriptor._elementSize = sizeof(NodeTransformData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize;
        bufferDescriptor._separateReadWrite = false;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        { 
            bufferDescriptor._name = Util::StringFormat("NODE_TRANSFORM_DATA_%s", TypeUtil::RenderStageToString(_stageFlag));
            _transformData = _context.newSB(bufferDescriptor);
        }

    }
    {// Node Material buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_CONCURRENT_MATERIALS;
        bufferDescriptor._elementSize = sizeof(NodeMaterialData);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize;
        bufferDescriptor._separateReadWrite = false;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        {
            bufferDescriptor._name = Util::StringFormat("NODE_MATERIAL_DATA_%s", TypeUtil::RenderStageToString(_stageFlag));
            _materialData = _context.newSB(bufferDescriptor);
        }

    }
    {// Indirect draw command buffer
        ShaderBufferDescriptor bufferDescriptor = {};
        bufferDescriptor._usage = ShaderBuffer::Usage::UNBOUND_BUFFER;
        bufferDescriptor._elementCount = getBufferFactor(_stageFlag) * Config::MAX_VISIBLE_NODES;
        bufferDescriptor._elementSize = sizeof(IndirectDrawCommand);
        bufferDescriptor._updateFrequency = BufferUpdateFrequency::OFTEN;
        bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
        bufferDescriptor._ringBufferLength = DataBufferRingSize;
        bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
        bufferDescriptor._separateReadWrite = false;
        {
            bufferDescriptor._name = Util::StringFormat("CMD_DATA_%s", TypeUtil::RenderStageToString(_stageFlag));
            _cmdBuffer = _context.newSB(bufferDescriptor);
        }
    }
}

RenderPass::BufferData RenderPass::getBufferData(const RenderStagePass& stagePass) const {
    assert(_stageFlag == stagePass._stage);

    U32 cmdBufferIdx = 0u;
    switch (_stageFlag) {
        case RenderStage::DISPLAY:
        case RenderStage::SHADOW:
            cmdBufferIdx = RenderStagePass::indexForStage(stagePass);
            break;

        case RenderStage::REFRACTION: { 
            switch (static_cast<RefractorType>(stagePass._variant)) {
                case RefractorType::PLANAR:
                    cmdBufferIdx = stagePass._index;
                    break;
                case RefractorType::COUNT:
                    DIVIDE_UNEXPECTED_CALL();
                    break;
            };
        } break;

        case RenderStage::REFLECTION: {
            switch (static_cast<ReflectorType>(stagePass._variant)) {
                case ReflectorType::PLANAR:  //We buffer for worst case scenario anyway (6 passes)
                case ReflectorType::CUBE:
                    cmdBufferIdx = stagePass._index * 6 + stagePass._pass;
                    break;
                case ReflectorType::COUNT:
                    DIVIDE_UNEXPECTED_CALL(); 
                    break;
            };
        } break;

        case RenderStage::COUNT:
            DIVIDE_UNEXPECTED_CALL();
            break;
    }

    BufferData ret;
    ret._cullCounterBuffer = _cullCounter;
    ret._transformBuffer = _transformData;
    ret._materialBuffer = _materialData;
    ret._commmandBuffer = _cmdBuffer;
    ret._lastCommandCount = &_lastCmdCount;
    ret._lastNodeCount = &_lastNodeCount;
    ret._materialData  = { _materialData->queueWriteIndex() , cmdBufferIdx * Config::MAX_CONCURRENT_MATERIALS };
    ret._transformData = { _transformData->queueWriteIndex(), cmdBufferIdx * Config::MAX_VISIBLE_NODES };
    ret._commandData   = { _cmdBuffer->queueWriteIndex()    , cmdBufferIdx * Config::MAX_VISIBLE_NODES };
    return ret;
}

void RenderPass::render(const Task& parentTask, const SceneRenderState& renderState, GFX::CommandBuffer& bufferInOut) const {
    OPTICK_EVENT();

    ACKNOWLEDGE_UNUSED(parentTask);

    switch(_stageFlag) {
        case RenderStage::DISPLAY: {
            OPTICK_EVENT("RenderPass - Main");

            static GFX::ClearRenderTargetCommand clearMainTarget = {};
            static RenderPassParams params = {};

            static bool initDrawCommands = false;
            if (!initDrawCommands) {
                RTClearDescriptor clearDescriptor = {};
                clearDescriptor.clearColours(true);
                clearDescriptor.clearDepth(true);
                clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::ALBEDO), false);
                clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);
                clearDescriptor.clearColour(to_U8(GFXDevice::ScreenTargets::EXTRA), true);
                clearMainTarget._descriptor = clearDescriptor;

                RTDrawDescriptor normalsAndDepthPolicy = {};
                normalsAndDepthPolicy.drawMask().disableAll();
                normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, true);
                normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::EXTRA), true);
                normalsAndDepthPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_base(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), true);

                RTDrawDescriptor mainPassPolicy = {};
                mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Depth, 0, false);
                mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::NORMALS_AND_VELOCITY), false);
                mainPassPolicy.drawMask().setEnabled(RTAttachmentType::Colour, to_U8(GFXDevice::ScreenTargets::EXTRA), false);

                params._passName = "MainRenderPass";
                params._stagePass = RenderStagePass{ _stageFlag, RenderPassType::COUNT };
                params._targetDescriptorPrePass = normalsAndDepthPolicy;
                params._targetDescriptorMainPass = mainPassPolicy;
                params._targetHIZ = RenderTargetID(RenderTargetUsage::HI_Z);

                initDrawCommands = true;
            }

            params._targetOIT = params._target._usage == RenderTargetUsage::SCREEN_MS ? RenderTargetID(RenderTargetUsage::OIT_MS) : RenderTargetID(RenderTargetUsage::OIT);
            params._camera = Attorney::SceneManagerCameraAccessor::playerCamera(_parent.parent().sceneManager());
            params._target = _context.renderTargetPool().screenTargetID();
            clearMainTarget._target = params._target;

            EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Main Display Pass" });
            EnqueueCommand(bufferInOut, clearMainTarget);

            _parent.doCustomPass(params, bufferInOut);

            EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
        } break;

        case RenderStage::SHADOW: {
            OPTICK_EVENT("RenderPass - Shadow");
            if (_config.rendering.shadowMapping.enabled) {
                constexpr bool useNegOneToOneDepth = false;
                SceneManager* mgr = _parent.parent().sceneManager();
                const Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(mgr);

                LightPool& lightPool = Attorney::SceneManagerRenderPass::lightPool(mgr);

                EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Shadow Render Stage" });

                if_constexpr(useNegOneToOneDepth) {
                    //ToDo: remove this and change lookup code
                    GFX::SetClippingStateCommand clipStateCmd;
                    clipStateCmd._negativeOneToOneDepth = true;
                    GFX::EnqueueCommand(bufferInOut, clipStateCmd);
                }
               lightPool.generateShadowMaps(*camera, bufferInOut);

               if_constexpr(useNegOneToOneDepth) {
                   GFX::SetClippingStateCommand clipStateCmd;
                   clipStateCmd._negativeOneToOneDepth = false;
                   GFX::EnqueueCommand(bufferInOut, clipStateCmd);
               }
                EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
            }
        } break;

        case RenderStage::REFLECTION: {
            static VisibleNodeList s_Nodes;

            EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Reflection Pass" });

            SceneManager* mgr = _parent.parent().sceneManager();
            Camera* camera = Attorney::SceneManagerCameraAccessor::playerCamera(mgr);
            SceneEnvironmentProbePool* envProbPool = Attorney::SceneRenderPass::getEnvProbes(mgr->getActiveScene());
            {
                SceneEnvironmentProbePool::Prepare(bufferInOut);

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
                                                                            bufferInOut))
                    {
                        ReflectionUtil::updateBudget();
                    }
                }
            }
            EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;

        case RenderStage::REFRACTION: {
            static VisibleNodeList s_Nodes;

            EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand{ "Refraction Pass" });

            OPTICK_EVENT("RenderPass - Refraction");
            // Get list of refractive nodes from the scene manager
            const SceneManager* mgr = _parent.parent().sceneManager();
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

            EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});

        } break;

        case RenderStage::COUNT:
            DIVIDE_UNEXPECTED_CALL();
            break;
    };
}

void RenderPass::postRender() const {
    OPTICK_EVENT();

    _transformData->incQueue();
    _materialData->incQueue();
    _cmdBuffer->incQueue();
}

};
#include "stdafx.h"

#include "Headers/EnvironmentProbeComponent.h"
#include "Headers/TransformComponent.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"

namespace Divide {

namespace {
    U16 g_maxEnvironmentProbes = 32;
};

vectorEASTL<bool> EnvironmentProbeComponent::s_availableSlices;
RenderTargetHandle EnvironmentProbeComponent::s_reflection;

EnvironmentProbeComponent::EnvironmentProbeComponent(SceneGraphNode& sgn, PlatformContext& context) 
    : BaseComponentType<EnvironmentProbeComponent, ComponentType::ENVIRONMENT_PROBE>(sgn, context),
      GUIDWrapper(),
      _aabb(vec3<F32>(-1), vec3<F32>(1)),
      _refaabb(vec3<F32>(-1), vec3<F32>(1))
{
    Scene& parentScene = sgn.sceneGraph().parentScene();
    assert(!s_availableSlices.empty());
        
    rtLayerIndex(allocateSlice());

    EditorComponentField typeField = {};
    typeField._name = "Is Local";
    typeField._dataGetter = [this](void* dataOut) {
        *static_cast<bool*>(dataOut) = _type == ProbeType::TYPE_LOCAL;
    };
    typeField._dataSetter = [this](const void* data) {
        _type = (*static_cast<const bool*>(data)) ? ProbeType::TYPE_LOCAL : ProbeType::TYPE_INFINITE;
    };
    typeField._type = EditorComponentFieldType::PUSH_TYPE;
    typeField._readOnly = false;
    typeField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(typeField));

    EditorComponentField bbField = {};
    bbField._name = "Bounding Box";
    bbField._data = &_refaabb;
    bbField._type = EditorComponentFieldType::BOUNDING_BOX;
    bbField._readOnly = false;
    getEditorComponent().registerField(std::move(bbField));

    EditorComponentField updateRateField = {};
    updateRateField._name = "Update Rate";
    updateRateField._tooltip = "[0...TARGET_FPS]. Every Nth frame. 0 = disabled;";
    updateRateField._data = &_updateRate; 
    updateRateField._type = EditorComponentFieldType::PUSH_TYPE;
    updateRateField._readOnly = false;
    updateRateField._range = { 0.f, Config::TARGET_FRAME_RATE };
    updateRateField._basicType = GFX::PushConstantType::UINT;
    updateRateField._basicTypeSize = GFX::PushConstantSize::BYTE;

    getEditorComponent().registerField(std::move(updateRateField));

    EditorComponentField showBoxField = {};
    showBoxField._name = "Show parallax correction AABB";
    showBoxField._data = &_showParallaxAABB;
    showBoxField._type = EditorComponentFieldType::PUSH_TYPE;
    showBoxField._readOnly = false;
    showBoxField._basicType = GFX::PushConstantType::BOOL;

    getEditorComponent().registerField(std::move(showBoxField));

    getEditorComponent().onChangedCbk([this](std::string_view) {
        _aabb.set(_refaabb.getMin(), _refaabb.getMax());
        _aabb.translate(_parentSGN.get<TransformComponent>()->getPosition());
    });

    Attorney::SceneEnvironmentProbeComponent::registerProbe(parentScene, *this);
}

EnvironmentProbeComponent::~EnvironmentProbeComponent()
{
    Attorney::SceneEnvironmentProbeComponent::unregisterProbe(_parentSGN.sceneGraph().parentScene(), *this);

    s_availableSlices[rtLayerIndex()] = false;
}

void EnvironmentProbeComponent::prepare(GFX::CommandBuffer& bufferInOut) {
    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.clearDepth(true);
    clearDescriptor.clearColours(true);
    clearDescriptor.resetToDefault(true);

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = s_reflection._targetID;
    clearMainTarget._descriptor = clearDescriptor;
    GFX::EnqueueCommand(bufferInOut, clearMainTarget);
}

void EnvironmentProbeComponent::onStartup(GFXDevice& context) {
    s_availableSlices.resize(g_maxEnvironmentProbes, false);
    // Reflection Targets
    SamplerDescriptor reflectionSampler = {};
    reflectionSampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.minFilter(TextureFilter::NEAREST);
    reflectionSampler.magFilter(TextureFilter::NEAREST);

    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_MAP, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
    environmentDescriptor.samplerDescriptor(reflectionSampler);
    environmentDescriptor.layerCount(g_maxEnvironmentProbes);


    TextureDescriptor depthDescriptor(TextureType::TEXTURE_CUBE_MAP, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);

    depthDescriptor.samplerDescriptor(reflectionSampler);

    vectorEASTL<RTAttachmentDescriptor> att = {
        { environmentDescriptor, RTAttachmentType::Colour, 0, DefaultColours::WHITE },
        { depthDescriptor, RTAttachmentType::Depth },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "EnvironmentProbe";
    desc._resolution = vec2<U16>(Config::REFLECTION_TARGET_RESOLUTION_ENVIRONMENT_PROBE);
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    s_reflection = context.renderTargetPool().allocateRT(RenderTargetUsage::ENVIRONMENT, desc);
}

void EnvironmentProbeComponent::onShutdown(GFXDevice& context)
{
    context.renderTargetPool().deallocateRT(s_reflection);
}

U16 EnvironmentProbeComponent::allocateSlice() {
    U16 i = 0;
    for (; i < g_maxEnvironmentProbes; ++i) {
        if (!s_availableSlices[i]) {
            s_availableSlices[i] = true;
            break;
        }
    }

    return i;
}

void EnvironmentProbeComponent::refresh(GFX::CommandBuffer& bufferInOut) {
    static std::array<Camera*, 6> cameras = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

    if (++_currentUpdateCall % _updateRate == 0) {
        _context.gfx().generateCubeMap(s_reflection._targetID,
                                       rtLayerIndex(),
                                       _aabb.getCenter(),
                                       vec2<F32>(0.1f, _aabb.getHalfExtent().length()),
                                       RenderStagePass(RenderStage::REFLECTION, RenderPassType::COUNT, to_U8(ReflectorType::CUBE), rtLayerIndex()),
                                       bufferInOut,
                                       cameras,
                                       true);
        _currentUpdateCall = 0;
    }
}

void EnvironmentProbeComponent::setBounds(const vec3<F32>& min, const vec3<F32>& max) {
    _aabb.set(min, max);
}

void EnvironmentProbeComponent::setBounds(const vec3<F32>& center, F32 radius) {
    _aabb.createFromSphere(center, radius);
}

void EnvironmentProbeComponent::setUpdateRate(U8 rate) {
    _updateRate = rate;
}

RenderTargetHandle EnvironmentProbeComponent::reflectionTarget() {
    return s_reflection;
}

F32 EnvironmentProbeComponent::distanceSqTo(const vec3<F32>& pos) const {
    return _aabb.getCenter().distanceSquared(pos);
}

void EnvironmentProbeComponent::PreUpdate(const U64 deltaTime) {
    using Parent = BaseComponentType<EnvironmentProbeComponent, ComponentType::ENVIRONMENT_PROBE>;
    if (_drawImpostor || showParallaxAABB()) {
        context().gfx().debugDrawSphere(_parentSGN.get<TransformComponent>()->getPosition(), 0.5f, DefaultColours::BLUE);
        context().gfx().debugDrawBox(_aabb.getMin(), _aabb.getMax(), DefaultColours::BLUE);
    }

    Parent::PreUpdate(deltaTime);
}

void EnvironmentProbeComponent::OnData(const ECS::CustomEvent& data) {
    if (data._type == ECS::CustomEvent::Type::TransformUpdated) {
        _aabb.set(_refaabb.getMin(), _refaabb.getMax());
        _aabb.translate(_parentSGN.get<TransformComponent>()->getPosition());
    } else if (data._type == ECS::CustomEvent::Type::EntityFlagChanged) {
        const SceneGraphNode::Flags flag = static_cast<SceneGraphNode::Flags>(std::any_cast<U32>(data._flag));
        if (flag == SceneGraphNode::Flags::SELECTED) {
            const auto [state, recursive] = std::any_cast<std::pair<bool, bool>>(data._userData);
            _drawImpostor = state;
        }
    }
}
}; //namespace Divide
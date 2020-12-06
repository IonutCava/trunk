#include "stdafx.h"

#include "Headers/SceneEnvironmentProbePool.h"

#include "Scenes/Headers/Scene.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "ECS/Components/Headers/EnvironmentProbeComponent.h"

namespace Divide {

vectorEASTL<DebugView_ptr> SceneEnvironmentProbePool::s_debugViews;
vectorEASTL<Camera*> SceneEnvironmentProbePool::s_probeCameras;

std::array<std::pair<bool, bool>, Config::MAX_REFLECTIVE_PROBES_PER_PASS> SceneEnvironmentProbePool::s_availableSlices;
RenderTargetHandle SceneEnvironmentProbePool::s_reflection;

SceneEnvironmentProbePool::SceneEnvironmentProbePool(Scene& parentScene)
    : SceneComponent(parentScene)
{
    for (U32 i = 0; i < 6; ++i) {
        s_probeCameras.emplace_back(Camera::createCamera<FreeFlyCamera>(Util::StringFormat("ProbeCamera_%d", i)));
    }
}

SceneEnvironmentProbePool::~SceneEnvironmentProbePool() 
{
    for (U32 i = 0; i < 6; ++i) {
        Camera::destroyCamera(s_probeCameras[i]);
    }
    s_probeCameras.clear();
}

U16 SceneEnvironmentProbePool::allocateSlice(bool lock) {
    for (U16 i = 0; i < Config::MAX_REFLECTIVE_PROBES_PER_PASS; ++i) {
        if (s_availableSlices[i].first) {
            s_availableSlices[i] = { false, lock };
            return i;
        }
    }

    DIVIDE_UNEXPECTED_CALL();
    return 0u;
}

void SceneEnvironmentProbePool::unlockSlice(const U16 slice) {
    s_availableSlices[slice] = { true, false };
}

void SceneEnvironmentProbePool::OnStartup(GFXDevice& context) {
    s_availableSlices.fill({ true, false });

    // Reflection Targets
    SamplerDescriptor reflectionSampler = {};
    reflectionSampler.wrapUVW(TextureWrap::CLAMP_TO_EDGE);
    reflectionSampler.magFilter(TextureFilter::LINEAR);
    reflectionSampler.minFilter(TextureFilter::LINEAR_MIPMAP_LINEAR);
    reflectionSampler.anisotropyLevel(context.context().config().rendering.anisotropicFilteringLevel);
    const size_t samplerHash = reflectionSampler.getHash();

    TextureDescriptor environmentDescriptor(TextureType::TEXTURE_CUBE_ARRAY, GFXImageFormat::RGB, GFXDataFormat::UNSIGNED_BYTE);
    environmentDescriptor.layerCount(Config::MAX_REFLECTIVE_PROBES_PER_PASS);
    environmentDescriptor.mipCount(1u);

    TextureDescriptor depthDescriptor(TextureType::TEXTURE_CUBE_ARRAY, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);
    depthDescriptor.layerCount(Config::MAX_REFLECTIVE_PROBES_PER_PASS);
    depthDescriptor.mipCount(1u);

    reflectionSampler.minFilter(TextureFilter::LINEAR);
    reflectionSampler.anisotropyLevel(0u);

    RTAttachmentDescriptors att = {
        { environmentDescriptor, samplerHash, RTAttachmentType::Colour },
        { depthDescriptor, samplerHash, RTAttachmentType::Depth },
    };

    const U32 reflectRes = nextPOW2(CLAMPED(to_U32(context.context().config().rendering.reflectionProbeResolution), 16u, 4096u) - 1u);

    RenderTargetDescriptor desc = {};
    desc._name = "EnvironmentProbe";
    desc._resolution.set(reflectRes, reflectRes);
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    s_reflection = context.renderTargetPool().allocateRT(RenderTargetUsage::ENVIRONMENT, desc);
}

void SceneEnvironmentProbePool::OnShutdown(GFXDevice& context) {
    context.renderTargetPool().deallocateRT(s_reflection);
}

RenderTargetHandle SceneEnvironmentProbePool::ReflectionTarget() {
    return s_reflection;
}

const EnvironmentProbeList& SceneEnvironmentProbePool::sortAndGetLocked(const vec3<F32>& position) {
    eastl::sort(begin(_envProbes),
                end(_envProbes),
                [&position](const auto& a, const auto& b) -> bool {
                    return a->distanceSqTo(position) < b->distanceSqTo(position);
                });

    return _envProbes;
}

void SceneEnvironmentProbePool::Prepare(GFX::CommandBuffer& bufferInOut) {
    for (U16 i = 0; i < Config::MAX_REFLECTIVE_PROBES_PER_PASS; ++i) {
        if (!s_availableSlices[i].second) {
            s_availableSlices[i].first = true;
        }
    }

    RTClearDescriptor clearDescriptor = {};
    clearDescriptor.clearDepth(true);
    // Do not reset already rendererd probes
    clearDescriptor.clearColours(false);
    clearDescriptor.resetToDefault(true);

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = s_reflection._targetID;
    clearMainTarget._descriptor = clearDescriptor;
    EnqueueCommand(bufferInOut, clearMainTarget);
}

void SceneEnvironmentProbePool::lockProbeList() const {
    _probeLock.lock();
}

void SceneEnvironmentProbePool::unlockProbeList() const {
    _probeLock.unlock();
}

const EnvironmentProbeList& SceneEnvironmentProbePool::getLocked() const {
    return _envProbes;
}

EnvironmentProbeComponent* SceneEnvironmentProbePool::registerProbe(EnvironmentProbeComponent* probe) {
    UniqueLock<SharedMutex> w_lock(_probeLock);
    _envProbes.emplace_back(probe);
    return probe;
}

void SceneEnvironmentProbePool::unregisterProbe(EnvironmentProbeComponent* probe) {
    if (probe != nullptr) {
        UniqueLock<SharedMutex> w_lock(_probeLock);
        I64 probeGUID = probe->getGUID();
        _envProbes.erase(eastl::remove_if(begin(_envProbes), end(_envProbes),
                                          [&probeGUID](const auto& p)
                                      -> bool { return p->getGUID() == probeGUID; }),
                         end(_envProbes));
    }

    if (probe == _debugProbe) {
        debugProbe(nullptr);
    }
}

void SceneEnvironmentProbePool::debugProbe(EnvironmentProbeComponent* probe) {
    _debugProbe = probe;
    // Remove old views
    if (!s_debugViews.empty()) {
        for (const DebugView_ptr& view : s_debugViews) {
            parentScene().context().gfx().removeDebugView(view.get());
        }
        s_debugViews.clear();
    }

    // Add new views if needed
    if (probe != nullptr) {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "fbPreview.glsl";
        fragModule._variant = "Cube";

        ShaderProgramDescriptor shaderDescriptor = {};
        shaderDescriptor._modules.push_back(vertModule);
        shaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor shadowPreviewShader("fbPreview.Cube");
        shadowPreviewShader.propertyDescriptor(shaderDescriptor);
        shadowPreviewShader.threaded(false);
        const ShaderProgram_ptr previewShader = CreateResource<ShaderProgram>(parentScene().resourceCache(), shadowPreviewShader);

        constexpr I32 Base = 10;
        for (U32 i = 0; i < 6; ++i) {
            DebugView_ptr probeView = std::make_shared<DebugView>(to_I16(std::numeric_limits<I16>::max() - 1 - 6 + i));
            probeView->_texture = ReflectionTarget()._rt->getAttachment(RTAttachmentType::Colour, 0).texture();
            probeView->_samplerHash = ReflectionTarget()._rt->getAttachment(RTAttachmentType::Colour, 0).samplerHash();
            probeView->_shader = previewShader;
            probeView->_shaderData.set(_ID("layer"), GFX::PushConstantType::INT, probe->rtLayerIndex());
            probeView->_shaderData.set(_ID("face"), GFX::PushConstantType::INT, i);
            probeView->_name = Util::StringFormat("CubeProbe_%d_face_%d", probe->rtLayerIndex(), i);
            probeView->_groupID = Base + probe->rtLayerIndex();
            s_debugViews.push_back(probeView);
        }
    }

    for (const DebugView_ptr& view : s_debugViews) {
        parentScene().context().gfx().addDebugView(view);
    }
}

} //namespace Divide
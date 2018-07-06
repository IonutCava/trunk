#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

SingleShadowMap::SingleShadowMap(Light* light, Camera* shadowCamera)
    : ShadowMap(light, shadowCamera, ShadowType::Single) {
    Console::printfn(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(),
                     "Single Shadow Map");
    ResourceDescriptor shadowPreviewShader("fbPreview.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    SamplerDescriptor depthMapSampler;
    depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);
    depthMapSampler._useRefCompare = true;
    depthMapSampler._cmpFunc = ComparisonFunction::LEQUAL;
    // Default filters, LINEAR is OK for this
    TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D,
                                         GFXImageFormat::DEPTH_COMPONENT,
                                         GFXDataFormat::UNSIGNED_INT);

    depthMapDescriptor.setSampler(depthMapSampler);
    _depthMap = GFX_DEVICE.newFB();
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::AttachmentType::Depth);
    _depthMap->toggleColorWrites(false);
}

SingleShadowMap::~SingleShadowMap() { RemoveResource(_previewDepthMapShader); }

void SingleShadowMap::init(ShadowMapInfo* const smi) {
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void SingleShadowMap::resolution(U16 resolution, U8 resolutionFactor) {
    U16 resolutionTemp = resolution * resolutionFactor;
    if (resolutionTemp != _resolution) {
        _resolution = resolutionTemp;
        // Initialize the FB's with a variable resolution
        Console::printfn(Locale::get("LIGHT_INIT_SHADOW_FB"),
                         _light->getGUID());
        _depthMap->Create(_resolution, _resolution);
    }

    ShadowMap::resolution(resolution, resolutionFactor);
}

void SingleShadowMap::render(SceneRenderState& renderState,
                             const DELEGATE_CBK<>& sceneRenderFunction) {
    /// Only if we have a valid callback;
    if (!sceneRenderFunction) {
        Console::errorfn(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"),
                         _light->getGUID());
        return;
    }

    renderState.getCameraMgr().pushActiveCamera(_shadowCamera, false);
    renderInternal(renderState, sceneRenderFunction);
    renderState.getCameraMgr().popActiveCamera(false);
}

void SingleShadowMap::renderInternal(
    const SceneRenderState& renderState,
    const DELEGATE_CBK<>& sceneRenderFunction) {
    _shadowCamera->lookAt(_light->getPosition(),
                          _light->getPosition() * _light->getDirection());
    _shadowCamera->setProjection(1.0f, 90.0f,
                                 vec2<F32>(1.0, _light->getRange()));
    _shadowCamera->renderLookAt();

    _depthMap->Begin(Framebuffer::defaultPolicy());
    // draw the scene
    GFX_DEVICE.getRenderer().render(sceneRenderFunction, renderState);
    // unbind the associated depth map
    _depthMap->End();
}

void SingleShadowMap::previewShadowMaps() {
    _depthMap->Bind(to_uint(ShaderProgram::TextureUsage::UNIT0));
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                          _previewDepthMapShader);
}
};
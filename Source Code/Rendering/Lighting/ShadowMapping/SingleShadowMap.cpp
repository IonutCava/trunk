#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

SingleShadowMap::SingleShadowMap(Light* light, Camera* shadowCamera) : ShadowMap(light, shadowCamera, SHADOW_TYPE_Single)
{
    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(), "Single Shadow Map");
    ResourceDescriptor shadowPreviewShader("fbPreview.LinearDepth");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    SamplerDescriptor depthMapSampler;
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);
    depthMapSampler._useRefCompare = true; //< Use compare function
    depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal

    TextureDescriptor depthMapDescriptor(TEXTURE_2D, DEPTH_COMPONENT, UNSIGNED_INT); ///Default filters, LINEAR is OK for this

    depthMapDescriptor.setSampler(depthMapSampler);
    _depthMap = GFX_DEVICE.newFB();
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
    _depthMap->toggleColorWrites(false);
}

SingleShadowMap::~SingleShadowMap()
{
    RemoveResource(_previewDepthMapShader);
}

void SingleShadowMap::init(ShadowMapInfo* const smi){
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void SingleShadowMap::resolution(U16 resolution, U8 resolutionFactor){
    U16 resolutionTemp = resolution * resolutionFactor;
    if (resolutionTemp != _resolution){
        _resolution = resolutionTemp;
        //Initialize the FB's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getGUID());
        _depthMap->Create(_resolution, _resolution);
    }
    ShadowMap::resolution(resolution, resolutionFactor);
}

void SingleShadowMap::render(SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    ///Only if we have a valid callback;
    if(sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getGUID());
        return;
    }
    renderState.getCameraMgr().pushActiveCamera(_shadowCamera, false);
    renderInternal(renderState, sceneRenderFunction);
    renderState.getCameraMgr().popActiveCamera(false);
}

void SingleShadowMap::renderInternal(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction) {
    _shadowCamera->lookAt(_light->getPosition(), _light->getPosition() * _light->getDirection());
    _shadowCamera->setProjection(1.0f, 90.0f, vec2<F32>(1.0, _light->getRange()));
    _shadowCamera->renderLookAt();

    _depthMap->Begin(Framebuffer::defaultPolicy());
        //draw the scene
        GFX_DEVICE.getRenderer()->render(sceneRenderFunction, renderState);
    //unbind the associated depth map
    _depthMap->End();
    LightManager::getInstance().registerShadowPass();
}

void SingleShadowMap::previewShadowMaps(){
    _depthMap->Bind(ShaderProgram::TEXTURE_UNIT0);
    GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _previewDepthMapShader);
}
#include "Headers/SingleShadowMap.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

SingleShadowMap::SingleShadowMap(Light* light) : ShadowMap(light, SHADOW_TYPE_Single)
{
    _maxResolution = 0;
    _resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
    CLAMP<F32>(_resolutionFactor,0.001f, 1.0f);
    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getId(), "Single Shadow Map");
    std::stringstream ss;
    ss << "Light " << (U32)light->getId() << " viewport " << 0;
    ResourceDescriptor mrt(ss.str());
    mrt.setFlag(true); //No default Material;
    _renderQuad = CreateResource<Quad3D>(mrt);
    ResourceDescriptor shadowPreviewShader("fbPreview");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _renderQuad->setCustomShader(_previewDepthMapShader);
    _renderQuad->renderInstance()->draw2D(true);

    SamplerDescriptor depthMapSampler;
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);
    depthMapSampler._useRefCompare = true; //< Use compare function
    depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal

    TextureDescriptor depthMapDescriptor(TEXTURE_2D,
                                         DEPTH_COMPONENT,
                                         DEPTH_COMPONENT,
                                         UNSIGNED_INT); ///Default filters, LINEAR is OK for this

    depthMapDescriptor.setSampler(depthMapSampler);
    _depthMap = GFX_DEVICE.newFB(FB_2D_DEPTH);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth);
    _depthMap->toggleColorWrites(false);
}

SingleShadowMap::~SingleShadowMap()
{
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_renderQuad);
}

void SingleShadowMap::init(ShadowMapInfo* const smi){
    resolution(smi->resolution(), smi->resolutionFactor());
    _init = true;
}

void SingleShadowMap::resolution(U16 resolution, F32 resolutionFactor){
    U8 resolutionFactorTemp = resolutionFactor;
    CLAMP<U8>(resolutionFactorTemp, 1, 4);
    U16 maxResolutionTemp = resolution;
    if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
        _resolutionFactor = resolutionFactorTemp;
        _maxResolution = maxResolutionTemp;
        //Initialize the FB's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getId());
        U16 shadowMapDimension = _maxResolution/_resolutionFactor;
        _depthMap->Create(shadowMapDimension,shadowMapDimension);
    }
    ShadowMap::resolution(resolution, resolutionFactor);
    _renderQuad->setDimensions(vec4<F32>(0,0,_depthMap->getWidth(),_depthMap->getHeight()));
}

void SingleShadowMap::render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    ///Only if we have a valid callback;
    if(sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
        return;
    }

    renderInternal(renderState, sceneRenderFunction);
}

void SingleShadowMap::renderInternal(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction) {
    //GFXDevice& gfx = GFX_DEVICE;
    ////Get our eye view
    //vec3<F32> eyePos = renderState.getCameraConst().getEye();
    ////For every depth map
    ////Lock our projection matrix so no changes will be permanent during the rest of the frame
    ////Lock our model view matrix so no camera transforms will be saved beyond this light's scope
    //gfx.lockMatrices();
    ////Set the camera to the light's view
    //_light->setCameraToLightView(eyePos);
    ////Set the appropriate projection
    //_light->getCameraToLightProjMatrix();
    ////bind the associated depth map
    //_depthMap->Begin(FrameBuffer::defaultPolicy());
    //    //draw the scene
    //    GFX_DEVICE.render(sceneRenderFunction, renderState);
    ////unbind the associated depth map
    //_depthMap->End();

    ////get all the required information (light VP matrix for example)
    ////and restore to the proper camera view
    //_light->setCameraToSceneView();
    ////Undo all modifications to the Projection and View Matrices
    //gfx.releaseMatrices();
    /////Restore our view frustum
    //Frustum::getInstance().Extract(eyePos);
}

void SingleShadowMap::previewShadowMaps(){
    _depthMap->Bind(0);
    _previewDepthMapShader->bind();
    _previewDepthMapShader->UniformTexture("tex",0);
    GFX_DEVICE.toggle2D(true);
        GFX_DEVICE.renderInViewport(vec4<I32>(0,0,256,256),
                                    boost::bind(&GFXDevice::renderInstance,
                                                DELEGATE_REF(GFX_DEVICE),
                                                _renderQuad->renderInstance()));
    GFX_DEVICE.toggle2D(false);
    _depthMap->Unbind(0);
}
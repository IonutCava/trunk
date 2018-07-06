#include "Headers/ParallelSplitShadowMaps.h"

#include "Scenes/Headers/SceneState.h"
#include "Core/Headers/ParamHandler.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Hardware/Video/Buffers/PixelBufferObject/Headers/PixelBufferObject.h"

#define JITTER_SIZE  16

PSShadowMaps::PSShadowMaps(Light* light) : ShadowMap(light, SHADOW_TYPE_PSSM)
{
    _maxResolution = 0;
    _resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
    CLAMP<F32>(_resolutionFactor,0.001f, 1.0f);
    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FBO"), light->getId(), "PSSM");
    //Initialize the FBO's with a variable resolution
    PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());

    ResourceDescriptor shadowPreviewShader("fboPreview.Layered");
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->UniformTexture("tex", 0);
    ResourceDescriptor blurShader("blur.Layered");
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurShader);
    _blurDepthMapShader->UniformTexture("texScreen", 0);
    _blurDepthMapShader->Uniform("kernelSize", 3);

    std::stringstream ss;
    ss << "Light " << (U32)light->getId() << " viewport ";
    ResourceDescriptor mrt(ss.str());
    mrt.setFlag(true); //No default Material;
    _renderQuad = CreateResource<Quad3D>(mrt);
    _renderQuad->renderInstance()->draw2D(true);
    _renderQuad->setCustomShader(_previewDepthMapShader);

    SamplerDescriptor depthMapSampler;
    depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, TEXTURE_FILTER_LINEAR);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(true);
    depthMapSampler.setAnisotrophy(8);

    TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY,
                                         RG,
                                         RG32F,
                                         FLOAT_32);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = GFX_DEVICE.newFBO(FBO_2D_ARRAY_COLOR);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _depthMap->toggleDepthBuffer(true); //<create a floating point depth buffer
    _depthMap->setClearColor(DefaultColors::WHITE());

    depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);

    _blurBuffer = GFX_DEVICE.newFBO(FBO_2D_ARRAY_COLOR);
    _blurBuffer->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());
}

PSShadowMaps::~PSShadowMaps()
{
    RemoveResource(_blurDepthMapShader);
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_renderQuad);
    SAFE_DELETE(_blurBuffer);
}

void PSShadowMaps::resolution(U16 resolution, const SceneRenderState& sceneRenderState){
    vec2<F32> zPlanes = Frustum::getInstance().getZPlanes();
    calculateSplitPoints(_light->getShadowMapInfo()->_numSplits, zPlanes.x,zPlanes.y);
    setOptimalAdjustFactor(0, 5);
    setOptimalAdjustFactor(1, 1);
    setOptimalAdjustFactor(2, 0);

    U8 resolutionFactorTemp = sceneRenderState.shadowMapResolutionFactor();
    CLAMP<U8>(resolutionFactorTemp, 1, 4);
    U16 maxResolutionTemp = resolution;
    if(resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
        _resolutionFactor = resolutionFactorTemp;
        _maxResolution = maxResolutionTemp;
        //Initialize the FBO's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());
        U16 shadowMapDimension = _maxResolution/_resolutionFactor;
        _depthMap->Create(shadowMapDimension,shadowMapDimension,_numSplits);
        _blurBuffer->Create(shadowMapDimension,shadowMapDimension,_numSplits);
        _renderQuad->setDimensions(vec4<F32>(0,0,shadowMapDimension,shadowMapDimension));
    }
    ShadowMap::resolution(resolution,sceneRenderState);
}

void PSShadowMaps::render(const SceneRenderState& renderState, boost::function0<void> sceneRenderFunction){
    //Only if we have a valid callback;
    if(sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
        return;
    }
    F32 ortho = LightManager::getInstance().getLigthOrthoHalfExtent();
    for(U8 i = 0; i < _numSplits; i++){
        _orthoPerPass[_numSplits - i - 1] = ortho / ((1.0f + i) * (1.0f + i) + i);
    }
    _callback = sceneRenderFunction;
    renderInternal(renderState);
}

void PSShadowMaps::setOptimalAdjustFactor(U8 index, F32 value){
    assert(index < _numSplits);
    if(_optAdjustFactor.size() <= index) _optAdjustFactor.push_back(value);
    else _optAdjustFactor[index] = value;
}

void PSShadowMaps::calculateSplitPoints(U8 splitCount, F32 nearDist, F32 farDist, F32 lambda){
    if (splitCount < 2){
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SPLIT_COUNT"), splitCount);
    }
    _numSplits = splitCount;
    _orthoPerPass.resize(_numSplits);
    _splitPadding = 5;
    _splitPoints.resize(splitCount + 1);
    _splitPoints[0] = nearDist;
    for (U8 i = 1; i < _numSplits; i++){
        F32 fraction = (F32)i / (F32)_numSplits;
        F32 splitPoint = lambda * nearDist * std::pow(farDist / nearDist, fraction) +
                         (1.0f - lambda) * (nearDist + fraction * (farDist - nearDist));

        _splitPoints[i] = splitPoint;
    }
    _splitPoints[splitCount] = farDist;
}

void PSShadowMaps::renderInternal(const SceneRenderState& renderState) const {
    // it's not hard to set a rendering callback, so just use that
    assert(!_callback.empty());

    GFXDevice& gfx = GFX_DEVICE;
    //Get our eye view
    const vec3<F32>& eyePos = renderState.getCameraConst().getEye();
    //For every depth map
    //Lock our projection matrix so no changes will be permanent during the rest of the frame
    //Lock our model view matrix so no camera transforms will be saved beyond this light's scope
    gfx.lockMatrices();
    //Set the camera to the light's view
    _light->setCameraToLightView(eyePos);
    //bind the associated depth map
    _depthMap->Begin(FrameBufferObject::defaultPolicy());
    //For each depth pass
    for(U8 i = 0; i < _numSplits; i++) {
        //Set the appropriate projection
        _light->renderFromLightView(i, _orthoPerPass[i]);
        _depthMap->DrawToLayer(TextureDescriptor::Color0, i);
        //draw the scene
        GFX_DEVICE.render(_callback, renderState);
    }
    _depthMap->End();

    //get all the required information (light VP matrix for example)
    //and restore to the proper camera view
    _light->setCameraToSceneView();
    //Undo all modifications to the Projection and View Matrices
    gfx.releaseMatrices();
    //Restore our view frustum
    Frustum::getInstance().Extract(eyePos);
}

void PSShadowMaps::postRender(){

    /*_renderQuad->setCustomShader(_blurDepthMapShader);
    _renderQuad->onDraw(DISPLAY_STAGE);

    _blurDepthMapShader->bind();
    _blurDepthMapShader->Uniform("horizontal", true);
    _blurDepthMapShader->Uniform("size", vec2<F32>(_depthMap->getWidth(), _depthMap->getHeight()));

    GFX_DEVICE.toggle2D(true);

    //Blur horizontally
    _blurBuffer->Begin(FrameBufferObject::defaultPolicy());
    _depthMap->Bind(0, TextureDescriptor::Color0);
    _depthMap->UpdateMipMaps(TextureDescriptor::Color0);
    for(U8 i = 0; i < _numSplits; i++) {
        _blurDepthMapShader->Uniform("layer", i);
        _blurBuffer->DrawToLayer(TextureDescriptor::Color0, i);
        GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
    }
    _depthMap->Unbind(0);
    _blurBuffer->End();

    _blurDepthMapShader->Uniform("horizontal", false);

    //Blur vertically
    _depthMap->Begin(FrameBufferObject::defaultPolicy());
    _blurBuffer->Bind(0, TextureDescriptor::Color0);
    for(U8 i = 0; i < _numSplits; i++) {
        _blurDepthMapShader->Uniform("layer", i);
        _depthMap->DrawToLayer(TextureDescriptor::Color0, i, false);
        GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
    }
    _blurBuffer->Unbind(0);
    _depthMap->End();

    GFX_DEVICE.toggle2D(false);

    _blurDepthMapShader->unbind();*/
}

void PSShadowMaps::previewShadowMaps(){
    _renderQuad->setCustomShader(_previewDepthMapShader);
    _renderQuad->onDraw(DISPLAY_STAGE);

    _depthMap->Bind(0, TextureDescriptor::Color0);
    _depthMap->UpdateMipMaps(TextureDescriptor::Color0);
    _previewDepthMapShader->bind();
    
    GFX_DEVICE.toggle2D(true);
        _previewDepthMapShader->Uniform("layer",0);
        GFX_DEVICE.renderInViewport(vec4<U32>(0,0,128,128),
                                    DELEGATE_BIND(&GFXDevice::renderInstance,
                                                DELEGATE_REF(GFX_DEVICE),
                                                DELEGATE_REF(_renderQuad->renderInstance())));
        _previewDepthMapShader->Uniform("layer",1);
        GFX_DEVICE.renderInViewport(vec4<U32>(130,0,128,128),
                                    DELEGATE_BIND(&GFXDevice::renderInstance,
                                                 DELEGATE_REF(GFX_DEVICE),
                                                 DELEGATE_REF(_renderQuad->renderInstance())));
        _previewDepthMapShader->Uniform("layer",2);
        GFX_DEVICE.renderInViewport(vec4<U32>(260,0,128,128),
                                    DELEGATE_BIND(&GFXDevice::renderInstance,
                                                DELEGATE_REF(GFX_DEVICE),
                                                DELEGATE_REF(_renderQuad->renderInstance())));
    GFX_DEVICE.toggle2D(false);
    _depthMap->Unbind(0);
}
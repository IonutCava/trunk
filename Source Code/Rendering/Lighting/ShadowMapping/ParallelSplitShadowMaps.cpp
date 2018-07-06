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
    ///Initialize the FBO's with a variable resolution
    PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FBO"), _light->getId());

    ResourceDescriptor shadowPreviewShader("fboPreview.Layered");
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    //ResourceDescriptor blurShader("blur.Box");
    //_blurDepthMapShader = CreateResource<ShaderProgram>(blurShader);
    std::stringstream ss;
    ss << "Light " << (U32)light->getId() << " viewport ";
    ResourceDescriptor mrt(ss.str());
    mrt.setFlag(true); //No default Material;
    _renderQuad = CreateResource<Quad3D>(mrt);
    _renderQuad->renderInstance()->draw2D(true);
    _renderQuad->setCustomShader(_previewDepthMapShader);

    //_jitterTexture = GFX_DEVICE.newPBO(PBO_TEXTURE_3D);
    //createJitterTexture(JITTER_SIZE,8,8);

    SamplerDescriptor depthMapSampler;
    //depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, TEXTURE_FILTER_LINEAR);
    depthMapSampler.setFilters(TEXTURE_FILTER_NEAREST);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    //depthMapSampler.toggleMipMaps(true);
    //depthMapSampler.setAnisotrophy(8);
    depthMapSampler._useRefCompare = true; //< Use compare function
    depthMapSampler._cmpFunc = CMP_FUNC_LEQUAL; //< Use less or equal
    depthMapSampler._depthCompareMode = LUMINANCE;

    TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY,
                                         DEPTH_COMPONENT,//RG,
                                         DEPTH_COMPONENT,//RG32F,
                                         UNSIGNED_INT);//FLOAT_32);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = GFX_DEVICE.newFBO(FBO_2D_ARRAY_DEPTH);//COLOR);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Depth/*Color0*/);
    //_depthMap->toggleDepthBuffer(true); //<create a floating point depth buffer
    _depthMap->toggleColorWrites(false);
    _depthMap->setClearColor(DefaultColors::WHITE());

    /*_blurBuffer = GFX_DEVICE.newFBO(FBO_2D_ARRAY_COLOR);
    _blurBuffer->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());*/
}

/*void PSShadowMaps::createJitterTexture(I32 size, I32 samples_u, I32 samples_v) {
    _jitterTexture->Create(size, size, samples_u * samples_v / 2, RGBA4, RGBA, UNSIGNED_BYTE);
    U8* data = (U8*)_jitterTexture->Begin();

    for (I32 i = 0; i<size; i++) {
        for (I32 j = 0; j<size; j++) {
            for (I32 k = 0; k<samples_u*samples_v/2; k++) {
                I32 x, y;
                vec4<F32> v;

                x = k % (samples_u / 2);
                y = (samples_v - 1) - k / (samples_u / 2);

                // generate points on a regular samples_u x samples_v rectangular grid
                v[0] = (F32)(x * 2 + 0.5f) / samples_u;
                v[1] = (F32)(y + 0.5f) / samples_v;
                v[2] = (F32)(x * 2 + 1 + 0.5f) / samples_u;
                v[3] = v[1];

                // jitter position
                v[0] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_u);
                v[1] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_v);
                v[2] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_u);
                v[3] += ((F32)rand() * 2 / RAND_MAX - 1) * (0.5f / samples_v);

                // warp to disk
                vec4<F32> d;
                d[0] = sqrtf(v[1]) * cosf(2 * M_PI * v[0]);
                d[1] = sqrtf(v[1]) * sinf(2 * M_PI * v[0]);
                d[2] = sqrtf(v[3]) * cosf(2 * M_PI * v[2]);
                d[3] = sqrtf(v[3]) * sinf(2 * M_PI * v[2]);

                data[(k * size * size + j * size + i) * 4 + 0] = (U8)(d[0] * 127);
                data[(k * size * size + j * size + i) * 4 + 1] = (U8)(d[1] * 127);
                data[(k * size * size + j * size + i) * 4 + 2] = (U8)(d[2] * 127);
                data[(k * size * size + j * size + i) * 4 + 3] = (U8)(d[3] * 127);
            }
        }
    }
    _jitterTexture->End();
}
*/
PSShadowMaps::~PSShadowMaps()
{
    //RemoveResource(_blurDepthMapShader);
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_renderQuad);
    //SAFE_DELETE(_blurBuffer);
    //SAFE_DELETE(_jitterTexture);
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
        //_blurBuffer->Create(shadowMapDimension,shadowMapDimension,_numSplits);
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
    _depthMap->Begin();
    //For each depth pass
    for(U8 i = 0; i < _numSplits; i++) {
        //Set the appropriate projection
        _light->renderFromLightView(i, _orthoPerPass[i]);
        _depthMap->DrawToLayer(TextureDescriptor::Depth/*Color0*/, i);
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
    _blurBuffer->Begin();
    _blurDepthMapShader->bind();

    _depthMap->Bind(0, TextureDescriptor::Color0);
        
    _blurDepthMapShader->UniformTexture("tex", 0);
    GFX_DEVICE.toggle2D(true);
    GFX_DEVICE.renderInViewport(vec4<U32>(0,0,_blurBuffer->getWidth(),_blurBuffer->getHeight()),
                                DELEGATE_BIND(&GFXDevice::renderInstance,
                                DELEGATE_REF(GFX_DEVICE),
                                DELEGATE_REF(_renderQuad->renderInstance())));
    GFX_DEVICE.toggle2D(false);
    _depthMap->Bind(0, TextureDescriptor::Color0);
    _blurBuffer->End();*/
}

void PSShadowMaps::previewShadowMaps(){
    _renderQuad->onDraw(DISPLAY_STAGE);

    _depthMap->Bind(0, TextureDescriptor::Depth/*Color0*/);
    //_depthMap->UpdateMipMaps(TextureDescriptor::Color0);
    _previewDepthMapShader->bind();
    _previewDepthMapShader->UniformTexture("tex", 0);
    
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
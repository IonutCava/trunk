#include "Headers/CascadedShadowMaps.h"

#include "Core/Headers/ParamHandler.h"

#include "Scenes/Headers/SceneState.h"

#include "Rendering/Headers/Frustum.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/ShaderManager.h"
#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Video/Headers/ImmediateModeEmulation.h"

#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

CascadedShadowMaps::CascadedShadowMaps(Light* light, F32 numSplits, F32 splitLogFactor) : ShadowMap(light, SHADOW_TYPE_CSM)
{
    _updateFrustum = false;
    _primitive = nullptr;
    _blurBuffer = nullptr;
    _blurDepthMapShader = nullptr;
    _splitLogFactor = splitLogFactor;
    _numSplits = numSplits;
    _splitDepths.resize(_numSplits + 1);
    _frustumCornersVS.resize(8);
    _frustumCornersLS.resize(8);
    _splitFrustumCornersVS.resize(8);
    _farFrustumCornersVS.resize(4);
    _maxResolution = 0;
    _frustum.resize(_numSplits);
    _shadowFloatValues.resize(_numSplits);
    _resolutionFactor = ParamHandler::getInstance().getParam<U8>("rendering.shadowResolutionFactor");
    CLAMP<F32>(_resolutionFactor, 0.1f, 8.0f);

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->UniformTexture("tex", 0);

    //ResourceDescriptor blurShader("blur.Layered");
    //blurShader.setThreadedLoading(false);
    //_blurDepthMapShader = CreateResource<ShaderProgram>(blurShader);
    //_blurDepthMapShader->UniformTexture("texScreen", 0);
    //_blurDepthMapShader->Uniform("kernelSize", 3);

    ResourceDescriptor mrt(std::string("Light " + Util::toString(light->getId()) + " viewport "));
    mrt.setFlag(true); //No default Material;
    _renderQuad = CreateResource<Quad3D>(mrt);
    _renderQuad->renderInstance()->draw2D(true);
    _renderQuad->renderInstance()->preDraw(true);
    _renderQuad->setCustomShader(_previewDepthMapShader);

    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getId(), "ECSM");
    SamplerDescriptor depthMapSampler;
    depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, TEXTURE_FILTER_LINEAR);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(true);
    depthMapSampler.setAnisotropy(4);
    TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY, RED, RED32F, FLOAT_32);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = GFX_DEVICE.newFB(FB_2D_ARRAY_COLOR);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _depthMap->toggleDepthBuffer(true); //<create a floating point depth buffer
    _depthMap->setClearColor(DefaultColors::WHITE());

    /*
    depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(false);

    _blurBuffer = GFX_DEVICE.newFB(FB_2D_ARRAY_COLOR);
    _blurBuffer->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());
    _blurDepthMapShader->Uniform("size", vec2<F32>(GFX_DEVICE.getScreenBuffer(0)->getWidth(), GFX_DEVICE.getScreenBuffer(0)->getHeight()));
    */
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    RemoveResource(_blurDepthMapShader);
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_renderQuad);
    SAFE_DELETE(_blurBuffer);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi){
    _numSplits = smi->numLayers();
    resolution(smi->resolution(), smi->resolutionFactor());
    _init = true;
}

void CascadedShadowMaps::resolution(U16 resolution, F32 resolutionFactor){
    F32 resolutionFactorTemp = resolutionFactor;
    CLAMP<F32>(resolutionFactorTemp, 0.1f, 8.0f);
    U16 maxResolutionTemp = resolution;
    if (resolutionFactorTemp != _resolutionFactor || _maxResolution != maxResolutionTemp){
        _resolutionFactor = resolutionFactorTemp;
        _maxResolution = maxResolutionTemp;
        //Initialize the FB's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getId());
        U16 shadowMapDimension = _maxResolution * _resolutionFactor;
        _depthMap->Create(shadowMapDimension, shadowMapDimension, _numSplits);
        vec2<U16> screenResolution = GFX_DEVICE.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution();
        _renderQuad->setDimensions(vec4<F32>(0, 0, screenResolution.width, screenResolution.height));
        //_blurBuffer->Create(screenResolution.width, screenResolution.height, _numSplits);
    }
    
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CascadedShadowMaps::render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    //Only if we have a valid callback;
    if (sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
        return;
    }
    GFX_DEVICE.getMatrix(VIEW_MATRIX, _viewMatrixCache);
    GFX_DEVICE.getMatrix(VIEW_INV_MATRIX, _viewInvMatrixCache);

    vec2<F32> zPlanes = Frustum::getInstance().getZPlanes();
    // split the view frustum and generate our lightViewProj matrices
    CalculateSplitDepths(renderState.getCameraConst(), vec2<F32>(0.25f, zPlanes.y));
#if defined(CSM_USE_LAYERED_RENDERING)
    // only cull the SceneGraph once
    RenderPassManager::getInstance().lock();
    // cull the SceneGraph and extract shadow casters and receivers
    extractShadowCastersAndReceivers(renderState);

    _depthMap->Begin(FrameBuffer::defaultPolicy());
        GFX_DEVICE.render(sceneRenderFunction, renderState);
    _depthMap->End();

    // unlock the render pass manager so we can cull the SceneGraph again
    RenderPassManager::getInstance().unlock(true);
#else
    GFX_DEVICE.lockMatrices();
    _depthMap->clearBuffers(false);
    _depthMap->Begin(FrameBuffer::defaultPolicy());
    _depthMap->clearBuffers(true);
    for (U8 i = 0; i < _numSplits; ++i){
        ApplyFrustumSplit(i, zPlanes);
        _depthMap->DrawToLayer(TextureDescriptor::Color0, i);
        GFX_DEVICE.render(sceneRenderFunction, renderState);
    }
    _depthMap->End();
    GFX_DEVICE.releaseMatrices();
    Frustum::getInstance().Extract();
#endif
}

void CascadedShadowMaps::CalculateSplitDepths(const Camera& cam, const vec2<F32>& zPlanes){
    cam.getFrustumCorners(_frustumCornersWS, zPlanes, RADIANS(Frustum::getInstance().getVerticalFoV()) + 0.3f);
    for (U8 i = 0; i < 8; ++i)
        _frustumCornersVS[i].set(_viewMatrixCache.transform(_frustumCornersWS[i]));
    for (U8 i = 0; i < 4; i++)
        _farFrustumCornersVS[i].set(_frustumCornersVS[i + 4]);

    I32 splitCount = (I32)_numSplits;
    F32 N = _numSplits;
    F32 nearPlane = zPlanes.x, farPlane = zPlanes.y;
    _splitDepths[0] = nearPlane;
    _splitDepths[_numSplits] = farPlane;
    for (I32 i = 1; i < splitCount; ++i){
        _splitDepths[i] = _splitLogFactor * nearPlane * (F32)pow(farPlane / nearPlane, i / N) + (1.0f - _splitLogFactor) * ((nearPlane + (i / N)) * (farPlane - nearPlane));
    }

#if defined(CSM_USE_LAYERED_RENDERING)
    for (U8 i = 0; i < _numSplits; ++i)
        ApplyFrustumSplit(i, zPlanes);
#endif
}

void CascadedShadowMaps::ApplyFrustumSplit(U8 pass, const vec2<F32>& zPlanes){
    frustum& f = _frustum[pass];
    F32 minZ = _splitDepths[pass], maxZ = _splitDepths[pass + 1];
    vec3<F32> frustumCentroid(0.0f);

    for (U8 i = 0; i < 4; i++){
        _splitFrustumCornersVS[i].set(_frustumCornersVS[i + 4] * (minZ / zPlanes.y));
        _frustumCornersWS[i].set(_viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));
        frustumCentroid += _frustumCornersWS[i]; 
        f.wsPoints[i].set(_frustumCornersWS[i]);
    }

    for (U8 i = 4; i < 8; i++){
        _splitFrustumCornersVS[i].set(_frustumCornersVS[i] * (maxZ / zPlanes.y));
        _frustumCornersWS[i].set(_viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));
        frustumCentroid += _frustumCornersWS[i];
        f.wsPoints[i].set(_frustumCornersWS[i]);
    }
    // Find the centroid
    frustumCentroid /= 8;
    
    // Position the shadow-caster camera so that it's looking at the centroid, and backed up in the direction of the sunlight
    F32 distFromCentroid = std::max((maxZ - minZ), _splitFrustumCornersVS[4].distance(_splitFrustumCornersVS[5])) + 50.0f;
    vec3<F32> currentEye = frustumCentroid - (_light->getPosition() * distFromCentroid);

#if defined(CSM_USE_LAYERED_RENDERING)
     mat4<F32> lightViewMatrix(GFX_DEVICE.getLookAt(currentEye, frustumCentroid));
#else
     mat4<F32> lightViewMatrix(GFX_DEVICE.lookAt(currentEye, frustumCentroid));
#endif
    
    // Determine the position of the frustum corners in light space
    for (U8 i = 0; i < 8; i++){
        _frustumCornersLS[i].set(lightViewMatrix.transform(_frustumCornersWS[i]));
        f.lsPoints[i].set(_frustumCornersLS[i]);
    }
    
    // Calculate an orthographic projection by sizing a bounding box 
    // to the frustum coordinates in light space
    BoundingBox frustumBox(_frustumCornersLS[0], _frustumCornersLS[0]);
    for (U8 i = 1; i < 8; i++)
        frustumBox.Add(_frustumCornersLS[i]);

#if !defined(CSM_USE_LAYERED_RENDERING)
    //for (const SceneGraphNode* caster : _casters)
    //  frustumBox.Add(caster->getBoundingBoxConst());
#endif

    vec3<F32> mins  = frustumBox.getMin();
    vec3<F32> maxes = frustumBox.getMax();

    // We snap the camera to 1 pixel increments so that moving the camera does not cause the shadows to jitter.
    // This is a matter of integer dividing by the world space size of a texel
    F32 diagonalLength = (_frustumCornersWS[0] - _frustumCornersWS[6]).length() + 2;
    F32 worldsUnitsPerTexel = diagonalLength / (F32)(_maxResolution / _resolutionFactor);

    vec3<F32> borderOffset = (vec3<F32>(diagonalLength) - (maxes - mins)) * 0.5f;
    maxes += borderOffset;
    mins  -= borderOffset;

    mins /= worldsUnitsPerTexel;

    mins.x = (F32)std::floor(mins.x);
    mins.y = (F32)std::floor(mins.y);
    mins.z = (F32)std::floor(mins.z);
    mins *= worldsUnitsPerTexel;

    maxes /= worldsUnitsPerTexel;
    maxes.x = (F32)std::floor(maxes.x);
    maxes.y = (F32)std::floor(maxes.y);
    maxes.z = (F32)std::floor(maxes.z);
    maxes *= worldsUnitsPerTexel;

    // Create an orthographic camera for use as a shadow caster
    const F32 nearClipOffset = 100.0f;
    vec2<F32> clipPlanes(-maxes.z - nearClipOffset, -mins.z);
    vec4<F32> clipRect(mins.x, maxes.x, mins.y, maxes.y);

#if defined(CSM_USE_LAYERED_RENDERING)
    mat4<F32> projMatrix(GFX_DEVICE.getOrthoProjection(clipRect, clipPlanes));
#else
    mat4<F32> projMatrix(GFX_DEVICE.setOrthoProjection(clipRect, clipPlanes));
    Frustum::getInstance().Extract(currentEye);
#endif

    _light->setVPMatrix(pass, lightViewMatrix * projMatrix);
}

#if defined(CSM_USE_LAYERED_RENDERING)
void CascadedShadowMaps::extractShadowCastersAndReceivers(const SceneRenderState& renderState){
    // get our eye view
    const vec3<F32>& eyePos = renderState.getCameraConst().getEye();
    vec3<F32> lightPosCulling(eyePos - _light->getPosition() * Config::DIRECTIONAL_LIGHT_DISTANCE);
    // lock view and projection
    GFX_DEVICE.lockMatrices();
    // set the view and projection matrices so that we can encapsulate most of our scene for frustum culling, centered around our camera position
    GFX_DEVICE.lookAt(lightPosCulling, eyePos);
    GFX_DEVICE.setOrthoProjection(UNIT_RECT * Config::DIRECTIONAL_LIGHT_DISTANCE, Frustum::getInstance().getZPlanes());
    // create a temporary frustum for culling the scene from the directional light's PoV
    Frustum::getInstance().Extract(lightPosCulling);
    // cull the scene and get a list of shadow casters and receivers
    SceneManager::getInstance().updateVisibleNodes();
    GET_ACTIVE_SCENEGRAPH()->getShadowCastersAndReceivers(_casters, _receivers, true);
    // undo all modifications to the Projection and View Matrices
    GFX_DEVICE.releaseMatrices();
}
#endif

void CascadedShadowMaps::postRender(){
    const mat4<F32>& projMatrixCache = GFX_DEVICE.getMatrix(PROJECTION_MATRIX);
     for (U8 i = 0; i < _numSplits; ++i) {
        _light->setFloatValue(i, 0.5f*(-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f);
        _light->setVPMatrix(i, _light->getVPMatrix(i) * _bias);
    }

    return; //needs more work
    /*_renderQuad->setCustomShader(_blurDepthMapShader);

    _blurDepthMapShader->bind();

    _blurDepthMapShader->Uniform("horizontal", true);
    GFX_DEVICE.toggle2D(true);

    //Blur horizontally
    _blurBuffer->Begin(FrameBuffer::defaultPolicy());
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
    _depthMap->Begin(FrameBuffer::defaultPolicy());
    _blurBuffer->Bind(0, TextureDescriptor::Color0);
    for(U8 i = 0; i < _numSplits; i++) {
    _blurDepthMapShader->Uniform("layer", i);
    _depthMap->DrawToLayer(TextureDescriptor::Color0, i, false);
    GFX_DEVICE.renderInstance(_renderQuad->renderInstance());
    }
    _blurBuffer->Unbind(0);
    _depthMap->End();

    GFX_DEVICE.toggle2D(false);

    _blurDepthMapShader->unbind();
    */
}

void CascadedShadowMaps::prepareDebugView(){
    static mat4<F32> frustaMat;
    frustaMat.setScale(0.5f);
    GFX_DEVICE.pushWorldMatrix(frustaMat, true);
}

void CascadedShadowMaps::releaseDebugView(){
    GFX_DEVICE.popWorldMatrix();
}

void CascadedShadowMaps::togglePreviewShadowMaps(bool state){
    ParamHandler::getInstance().setParam("rendering.debug.showSplits", state);
    _updateFrustum = !state;
}

void CascadedShadowMaps::previewShadowMaps(){
    if (!_primitive){
        _primitive = GFX_DEVICE.createPrimitive(false);
        _primitive->setRenderStates(DELEGATE_BIND(&CascadedShadowMaps::prepareDebugView, this),
                                    DELEGATE_BIND(&CascadedShadowMaps::releaseDebugView, this));
        _updateFrustum = true;
    }

    _previewDepthMapShader->bind();
    _renderQuad->setCustomShader(_previewDepthMapShader);
    _previewDepthMapShader->Uniform("far_plane", 20.0f);
    _depthMap->Bind();
    _depthMap->UpdateMipMaps(TextureDescriptor::Color0);

    GFX_DEVICE.toggle2D(true);
    for (U8 i = 0; i < _numSplits; ++i){
        _previewDepthMapShader->Uniform("layer", i);
        _previewDepthMapShader->Uniform("zPlanes", vec2<F32>(_splitDepths[i], _splitDepths[i + 1]));
        GFX_DEVICE.renderInViewport(vec4<I32>(130 * i, 0, 128, 128),
            DELEGATE_BIND(&GFXDevice::renderInstance,
            DELEGATE_REF(GFX_DEVICE),
            _renderQuad->renderInstance()));
    }
    GFX_DEVICE.toggle2D(false);
    _depthMap->Unbind();
    
    if (_updateFrustum && false){
        _primitive->beginBatch();
        _primitive->attribute4ub("inColorData", vec4<U8>(255, 255, 0, 255));
        _primitive->attribute1i("inSkipMVP", 0);
        drawFrustum(false);
        drawFrustum(true);
        _primitive->endBatch();
        _updateFrustum = false;
    }

}

void CascadedShadowMaps::drawFrustum(bool lightFrustum){
    static vec4<U8> colors[5];
    static bool colors_set = false;
    if (!colors_set){
        colors[0].set(255, 0, 0, 255);
        colors[1].set(0, 255, 0, 255);
        colors[2].set(0, 0, 255, 255);
        colors[3].set(255, 255, 0, 255);
        colors[4].set(255, 255, 255, 255);
        colors_set = true;
    }

    for (U8 i = 0; i<_numSplits; ++i){
        frustum& f = _frustum[i];
        vec3<F32>* points = (lightFrustum ? f.lsPoints : f.wsPoints);
         _primitive->begin(LINE_LOOP);
        _primitive->attribute4ub("inColorData", colors[i]);
        for (U8 j = 0; j < 4; ++j)
            _primitive->vertex(points[j]);
        _primitive->end();
        _primitive->begin(LINE_LOOP);
        _primitive->attribute4ub("inColorData", colors[i]);
        for (U8 j = 4; j < 8; ++j)
            _primitive->vertex(points[j]);
        _primitive->end();
    }

    for (U8 j = 0; j<4; j++) {
        _primitive->begin(LINE_STRIP);
        _primitive->attribute4ub("inColorData", colors[4]);
        for (U8 i = 0; i < _numSplits; ++i){
            const frustum& f = _frustum[i];
            _primitive->vertex(lightFrustum ? f.lsPoints[j] : f.wsPoints[j]);
        }
        const frustum& f = _frustum[(I32)(_numSplits)-1];
        _primitive->vertex(lightFrustum ? f.lsPoints[j + 4] : f.wsPoints[j + 4]);
        _primitive->end();
    }
}


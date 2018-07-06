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

CascadedShadowMaps::CascadedShadowMaps(Light* light, F32 numSplits) : ShadowMap(light, SHADOW_TYPE_CSM),
                                                                      _gfxDevice(GFX_DEVICE)
{
    _dirLight = dynamic_cast<DirectionalLight*>(_light);
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _numSplits = numSplits;
    _splitDepths.resize(_numSplits + 1);
    _frustumCornersWS.resize(8);
    _frustumCornersVS.resize(8);
    _frustumCornersLS.resize(8);
    _splitFrustumCornersVS.resize(8);
    _shadowFloatValues.resize(_numSplits);
    _horizBlur.resize(1);
    _vertBlur.resize(1);
    _renderPolicy = New FrameBuffer::FrameBufferTarget(FrameBuffer::defaultPolicy());
    _renderPolicy->_clearBuffersOnBind = false; //<we clear the FB on each face draw call, not on Begin()

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->UniformTexture("tex", 0);
    _previewDepthMapShader->Uniform("far_plane", 2000.0f);

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);
    _blurDepthMapShader->UniformTexture("texScreen", 0);

    PRINT_FN(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getId(), "ECSM");
    SamplerDescriptor depthMapSampler;
    depthMapSampler.setFilters(TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR, TEXTURE_FILTER_LINEAR);
    depthMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    depthMapSampler.toggleMipMaps(true);
    depthMapSampler.setAnisotropy(0);
    TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY_MS, RG, RG32F, FLOAT_32);
    depthMapDescriptor.setLayerCount(_numSplits);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = _gfxDevice.newFB(false);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _depthMap->toggleDepthBuffer(true); //<create a floating point depth buffer
    _depthMap->setClearColor(DefaultColors::WHITE());

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TEXTURE_FILTER_LINEAR, TEXTURE_FILTER_LINEAR);
    blurMapSampler.setWrapMode(TEXTURE_CLAMP_TO_EDGE);
    blurMapSampler.toggleMipMaps(false);
    depthMapSampler.setAnisotropy(0);
    TextureDescriptor blurMapDescriptor(TEXTURE_2D_ARRAY, RG, RG32F, FLOAT_32);
    blurMapDescriptor.setLayerCount(_numSplits);
    blurMapDescriptor.setSampler(blurMapSampler);
    
    _blurBuffer = _gfxDevice.newFB(false);
    _blurBuffer->AddAttachment(blurMapDescriptor, TextureDescriptor::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_blurDepthMapShader);
    SAFE_DELETE(_blurBuffer);
    SAFE_DELETE(_renderPolicy);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi){
    _numSplits = smi->numLayers();
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void CascadedShadowMaps::resolution(U16 resolution, U8 resolutionFactor){
    U16 tempResolution = resolution * resolutionFactor;
    if(_resolution != tempResolution){
        _resolution = tempResolution;
        //Initialize the FB's with a variable resolution
        PRINT_FN(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getId());
        _depthMap->Create(_resolution, _resolution);
        vec2<U16> screenResolution = _gfxDevice.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution();
        _horizBlur[0] = _blurDepthMapShader->GetSubroutineIndex(GEOMETRY_SHADER, "computeCoordsH");
        _vertBlur[0]  = _blurDepthMapShader->GetSubroutineIndex(GEOMETRY_SHADER, "computeCoordsV");
        _blurBuffer->Create(_resolution, _resolution);
        updateResolution(screenResolution.width, screenResolution.height);
    }
    _clipRect.resize(_numSplits);
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CascadedShadowMaps::updateResolution(I32 newWidth, I32 newHeight){
    ShadowMap::updateResolution(newWidth, newHeight);
}

void CascadedShadowMaps::render(const SceneRenderState& renderState, const DELEGATE_CBK& sceneRenderFunction){
    //Only if we have a valid callback;
    if (sceneRenderFunction.empty()) {
        ERROR_FN(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getId());
        return;
    }
    _gfxDevice.getMatrix(VIEW_MATRIX, _viewMatrixCache);
    _gfxDevice.getMatrix(VIEW_INV_MATRIX, _viewInvMatrixCache);
    
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _sceneZPlanes = Frustum::getInstance().getZPlanes();
    // split the view frustum and generate our lightViewProj matrices
    CalculateSplitDepths(renderState.getCameraConst());
    _gfxDevice.lockMatrices();
    _depthMap->Begin(*_renderPolicy);
    for (U8 i = 0; i < _numSplits; ++i){
        ApplyFrustumSplit(i);
        _depthMap->DrawToLayer(TextureDescriptor::Color0, i, true);
        _gfxDevice.render(sceneRenderFunction, renderState);
    }
    _depthMap->End();
    _gfxDevice.releaseMatrices();
    Frustum::getInstance().Extract();

    _gfxDevice.setLight(_light, true);
}

void CascadedShadowMaps::CalculateSplitDepths(const Camera& cam){
    const mat4<F32>& projMatrixCache = _gfxDevice.getMatrix(PROJECTION_MATRIX);
    for (U8 i = 0; i < 8; ++i){
        _frustumCornersWS[i].set(Frustum::getInstance().getPoint(i));
        _frustumCornersVS[i].set(_viewMatrixCache.transform(_frustumCornersWS[i]));
    }

    F32 N = _numSplits;
    F32 nearPlane = _sceneZPlanes.x;
    F32 farPlane  = _sceneZPlanes.y;
    _splitDepths[0] = nearPlane;
    _splitDepths[_numSplits] = farPlane;
    for (I32 i = 1; i < (I32)_numSplits; ++i)
        _splitDepths[i] = _splitLogFactor * nearPlane * (F32)pow(farPlane / nearPlane, i / N) + (1.0f - _splitLogFactor) * ((nearPlane + (i / N)) * (farPlane - nearPlane));
    for (U8 i = 0; i < _numSplits; ++i)
        _light->setFloatValue(i, 0.5f*(-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f);
}

void CascadedShadowMaps::ApplyFrustumSplit(U8 pass){
    mat4<F32>& viewMatrix = _shadowViewMatrices[pass];
    mat4<F32>& projMatrix = _shadowProjMatrices[pass];

    F32 minZ = _splitDepths[pass];
    F32 maxZ = _splitDepths[pass + 1];

    for (U8 i = 0; i < 4; i++)
        _splitFrustumCornersVS[i] = _frustumCornersVS[i + 4] * (minZ / _sceneZPlanes.y);

    for (U8 i = 4; i < 8; i++)
        _splitFrustumCornersVS[i] = _frustumCornersVS[i] * (maxZ / _sceneZPlanes.y);

    for (U8 i = 0; i < 8; i++)
        _frustumCornersWS[i].set(_viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));

    vec3<F32> frustumCentroid(0.0f);
    for (U8 i = 0; i < 8; ++i)
        frustumCentroid += _frustumCornersWS[i];
    // Find the centroid
    frustumCentroid /= 8;

    // Position the shadow-caster camera so that it's looking at the centroid, and backed up in the direction of the sunlight
    F32 distFromCentroid = std::max((maxZ - minZ), _splitFrustumCornersVS[4].distance(_splitFrustumCornersVS[5])) + _nearClipOffset;
    vec3<F32> currentEye = frustumCentroid - (_light->getPosition() * distFromCentroid);

    viewMatrix.set(_gfxDevice.lookAt(currentEye, frustumCentroid));
    // Determine the position of the frustum corners in light space
    for (U8 i = 0; i < 8; i++){
        _frustumCornersLS[i].set(viewMatrix.transform(_frustumCornersWS[i]));
    }
    // Create an orthographic camera for use as a shadow caster
    vec2<F32> clipPlanes;

    if (_dirLight->csmStabilize()){
        BoundingSphere frustumSphere(_frustumCornersLS);
        _clipRect[pass].set(UNIT_RECT * frustumSphere.getRadius());
        clipPlanes.set(0.0f, frustumSphere.getDiameter());
        clipPlanes += _nearClipOffset;
    }else{
        // Calculate an orthographic projection by sizing a bounding box to the frustum coordinates in light space
        BoundingBox frustumBox(_frustumCornersLS);
        vec3<F32> maxes = frustumBox.getMax();
        vec3<F32> mins  = frustumBox.getMin();
         // Create an orthographic camera for use as a shadow caster
        clipPlanes.set(-maxes.z - _nearClipOffset, -mins.z);
        _clipRect[pass].set(mins.x, maxes.x, mins.y, maxes.y);
    }

    projMatrix.set(_gfxDevice.setOrthoProjection(_clipRect[pass], clipPlanes));
    Frustum::getInstance().Extract(currentEye);

    mat4<F32> lightViewProj = viewMatrix * projMatrix;

    // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
    F32 halfShadowMapSize = (_resolution) * 0.5f;
    vec3<F32> testPoint = lightViewProj.transform(VECTOR3_ZERO) * halfShadowMapSize;
    vec3<F32> testPointRounded(testPoint); testPointRounded.round();
    vec3<F32> rounding = (testPointRounded - testPoint) / halfShadowMapSize;
    lightViewProj *= mat4<F32>(rounding.x, rounding.y, 0.0f);

    _light->setVPMatrix(pass, lightViewProj * _bias);
    _light->setLightPos(pass, currentEye);
}

void CascadedShadowMaps::postRender(){
    if (_gfxDevice.shadowDetailLevel() == DETAIL_LOW)
        return;

    

    _gfxDevice.toggle2D(true);

    _blurDepthMapShader->bind();
    _blurBuffer->Begin(*_renderPolicy);
    _depthMap->Bind();

    F32 baseBlurSize = 1.0f / _resolution;
    //Blur layer 0 horizontally
    _blurDepthMapShader->SetSubroutines(GEOMETRY_SHADER, _horizBlur);
    _blurDepthMapShader->Uniform("layer", 0);
    _blurDepthMapShader->Uniform("blurSize", baseBlurSize);
    _blurBuffer->DrawToLayer(TextureDescriptor::Color0, 0, false);
    _gfxDevice.drawPoints(1);
    _depthMap->Begin(*_renderPolicy);
    _blurBuffer->Bind();

    //Blur layer 0 vertically
    _blurDepthMapShader->SetSubroutines(GEOMETRY_SHADER, _vertBlur);
    _blurDepthMapShader->Uniform("layer", 0);
    _blurDepthMapShader->Uniform("blurSize", baseBlurSize);
    _depthMap->DrawToLayer(TextureDescriptor::Color0, 0, false);
    _gfxDevice.drawPoints(1);
    _blurBuffer->Unbind();
    _depthMap->End();

    _gfxDevice.toggle2D(false);
}

void CascadedShadowMaps::togglePreviewShadowMaps(bool state){
    ParamHandler::getInstance().setParam("rendering.debug.showSplits", state);
}

void CascadedShadowMaps::previewShadowMaps(){
    _gfxDevice.toggle2D(true);
    _previewDepthMapShader->bind();
    _depthMap->Bind();
    _depthMap->UpdateMipMaps(TextureDescriptor::Color0);
    for (U8 i = 0; i < _numSplits; ++i){
        _previewDepthMapShader->Uniform("layer", i);
        _previewDepthMapShader->Uniform("zPlanes", vec2<F32>(_splitDepths[i], _splitDepths[i + 1]));
        _gfxDevice.renderInViewport(vec4<I32>(130 * i, 0, 128, 128), DELEGATE_BIND(&GFXDevice::drawPoints, DELEGATE_REF(_gfxDevice), 1));
    }
    _depthMap->Unbind();
    _gfxDevice.toggle2D(false);
}
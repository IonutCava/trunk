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
    _frustumCornersVS.resize(8);
    _frustumCornersLS.resize(8);
    _splitFrustumCornersVS.resize(8);
    _farFrustumCornersVS.resize(4);
    _shadowFloatValues.resize(_numSplits);

    _renderPolicy = New FrameBuffer::FrameBufferTarget(FrameBuffer::defaultPolicy());
    _renderPolicy->_clearBuffersOnBind = false; //<we clear the FB on each face draw call, not on Begin()

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->UniformTexture("tex", 0);

    ResourceDescriptor blurDepthMapShader("depthPass.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);

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
    depthMapSampler.setAnisotropy(8);
    TextureDescriptor depthMapDescriptor(TEXTURE_2D_ARRAY_MS, RG, RG32F, FLOAT_32);
    depthMapDescriptor.setLayerCount(_numSplits);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = _gfxDevice.newFB(false);
    _depthMap->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _depthMap->toggleDepthBuffer(true); //<create a floating point depth buffer
    _depthMap->setClearColor(DefaultColors::WHITE());

    _blurBuffer = _gfxDevice.newFB(false);
    _blurBuffer->AddAttachment(depthMapDescriptor, TextureDescriptor::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_blurDepthMapShader);
    RemoveResource(_renderQuad);
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
        _blurBuffer->Create(_resolution, _resolution);
        vec2<U16> screenResolution = _gfxDevice.getRenderTarget(GFXDevice::RENDER_TARGET_SCREEN)->getResolution();
        _blurDepthMapShader->Uniform("blurSize", 1.0f / _resolution);
        _blurDepthMapShader->UniformTexture("shadowMap", 0);
        updateResolution(screenResolution.width, screenResolution.height);
    }
    
    ShadowMap::resolution(resolution, resolutionFactor);
}

void CascadedShadowMaps::updateResolution(I32 newWidth, I32 newHeight){
    _renderQuad->setDimensions(vec4<F32>(0, 0, newWidth, newHeight));
    _blurDepthMapShader->Uniform("size", vec2<F32>(newWidth, newHeight));
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
    F32 worldRadius = GET_ACTIVE_SCENEGRAPH()->getRoot()->getBoundingSphereConst().getRadius();
    _sceneZPlanes.x = std::max(_sceneZPlanes.x, 1.0f);
    _sceneZPlanes.y = std::min(_sceneZPlanes.y, worldRadius + _nearClipOffset);
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
}

void CascadedShadowMaps::CalculateSplitDepths(const Camera& cam){
    cam.getFrustumCorners(_frustumCornersWS, _sceneZPlanes, RADIANS(Frustum::getInstance().getVerticalFoV()) + 0.2f);

    for (U8 i = 0; i < 8; ++i)
        _frustumCornersVS[i].set(_viewMatrixCache.transform(_frustumCornersWS[i]));
    for (U8 i = 0; i < 4; i++)
        _farFrustumCornersVS[i].set(_frustumCornersVS[i + 4]);

    F32 N = _numSplits;
    F32 nearPlane = _sceneZPlanes.x;
    F32 farPlane  = _sceneZPlanes.y;
    _splitDepths[0] = nearPlane;
    _splitDepths[_numSplits] = farPlane;
    for (I32 i = 1; i < (I32)_numSplits; ++i){
        _splitDepths[i] = _splitLogFactor * nearPlane * (F32)pow(farPlane / nearPlane, i / N) + (1.0f - _splitLogFactor) * ((nearPlane + (i / N)) * (farPlane - nearPlane));
    }
}

void CascadedShadowMaps::ApplyFrustumSplit(U8 pass){
    mat4<F32>& viewMatrix = _shadowViewMatrices[pass];
    mat4<F32>& projMatrix = _shadowProjMatrices[pass];

    F32 minZ = _splitDepths[pass];
    F32 maxZ = _splitDepths[pass + 1];

    for (U8 i = 0; i < 4; i++){
        _splitFrustumCornersVS[i].set(_frustumCornersVS[i + 4] * (minZ / _sceneZPlanes.y));
        _frustumCornersWS[i].set(_viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));
    }

    for (U8 i = 4; i < 8; i++){
        _splitFrustumCornersVS[i].set(_frustumCornersVS[i] * (maxZ / _sceneZPlanes.y));
        _frustumCornersWS[i].set(_viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));
    }

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
    vec4<F32> clipRect;

    if (_dirLight->csmStabilize()){
        BoundingSphere frustumSphere(_frustumCornersLS);
        clipRect.set(UNIT_RECT * frustumSphere.getRadius());
        clipPlanes.set(0.0f, frustumSphere.getDiameter());
        clipPlanes += _nearClipOffset;
    }else{
        // Calculate an orthographic projection by sizing a bounding box to the frustum coordinates in light space
        BoundingBox frustumBox(_frustumCornersLS);
        vec3<F32> maxes = frustumBox.getMax();
        vec3<F32> mins  = frustumBox.getMin();
         // Create an orthographic camera for use as a shadow caster
        clipPlanes.set(-maxes.z - _nearClipOffset, -mins.z);
        clipRect.set(mins.x, maxes.x, mins.y, maxes.y);
    }

    projMatrix.set(_gfxDevice.setOrthoProjection(clipRect, clipPlanes));
    Frustum::getInstance().Extract(currentEye);

    mat4<F32> lightViewProj = viewMatrix * projMatrix;

    // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
    F32 halfShadowMapSize = (_resolution) * 0.5f;
    vec3<F32> testPoint = lightViewProj.transform(VECTOR3_ZERO) * halfShadowMapSize;
    vec3<F32> testPointRounded(testPoint); testPointRounded.round();
    vec3<F32> rounding = (testPointRounded - testPoint) / halfShadowMapSize;
    lightViewProj *= mat4<F32>(rounding.x, rounding.y, 0.0f);

    _light->setVPMatrix(pass, lightViewProj);
}

void CascadedShadowMaps::postRender(){
    const mat4<F32>& projMatrixCache = _gfxDevice.getMatrix(PROJECTION_MATRIX);
    for (U8 i = 0; i < _numSplits; ++i) {
        _light->setFloatValue(i, 0.5f*(-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f);
        _light->setVPMatrix(i, _light->getVPMatrix(i) * _bias);
    }

    if (_gfxDevice.shadowDetailLevel() == DETAIL_LOW)
        return;

    _gfxDevice.toggle2D(true);

    RenderInstance* quadRI = _renderQuad->renderInstance();
    _renderQuad->setCustomShader(_blurDepthMapShader);
    _blurDepthMapShader->bind();
    _blurDepthMapShader->Uniform("horizontal", true);
    _blurDepthMapShader->Uniform("size", Application::getInstance().getResolution());
     //Blur horizontally
    _blurBuffer->Begin(*_renderPolicy);
     //bright spots
    _depthMap->Bind(0);
    for (U8 i = 0; i < _numSplits; ++i){
        _blurDepthMapShader->Uniform("layer", i);
        _blurBuffer->DrawToLayer(TextureDescriptor::Color0, i, false);
        _gfxDevice.renderInstance(quadRI);
    }
    _blurDepthMapShader->Uniform("horizontal", false);
    //Blur vertically
    _depthMap->Begin(*_renderPolicy);
    //horizontally blurred bright spots
    _blurBuffer->Bind(0);
    for (U8 i = 0; i < _numSplits; ++i){
        _blurDepthMapShader->Uniform("layer", i);
        _depthMap->DrawToLayer(TextureDescriptor::Color0, i, false);
        _gfxDevice.renderInstance(quadRI);
    }

    // clear states
    _blurBuffer->Unbind(0);
    _depthMap->End();
    _gfxDevice.toggle2D(false);
}

void CascadedShadowMaps::togglePreviewShadowMaps(bool state){
    ParamHandler::getInstance().setParam("rendering.debug.showSplits", state);
}

void CascadedShadowMaps::previewShadowMaps(){
    _previewDepthMapShader->bind();
    _renderQuad->setCustomShader(_previewDepthMapShader);
    _previewDepthMapShader->Uniform("far_plane", 2000.0f);
    _depthMap->Bind();
    _depthMap->UpdateMipMaps(TextureDescriptor::Color0);
    _gfxDevice.toggle2D(true);
    for (U8 i = 0; i < _numSplits; ++i){
        _previewDepthMapShader->Uniform("layer", i);
        _previewDepthMapShader->Uniform("zPlanes", vec2<F32>(_splitDepths[i], _splitDepths[i + 1]));
        _gfxDevice.renderInViewport(vec4<I32>(130 * i, 0, 128, 128), DELEGATE_BIND(&GFXDevice::renderInstance, DELEGATE_REF(_gfxDevice), _renderQuad->renderInstance()));
    }
    _gfxDevice.toggle2D(false);
    _depthMap->Unbind();
    
}
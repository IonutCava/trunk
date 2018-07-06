#include "Headers/CascadedShadowMaps.h"

#include "Core/Headers/ParamHandler.h"

#include "Scenes/Headers/SceneState.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"

#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

CascadedShadowMaps::CascadedShadowMaps(Light* light, Camera* shadowCamera,
                                       F32 numSplits)
    : ShadowMap(light, shadowCamera, ShadowType::LAYERED)
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
    _horizBlur = 0;
    _vertBlur = 0;
    _renderPolicy = MemoryManager_NEW Framebuffer::FramebufferTarget(
        Framebuffer::defaultPolicy());
    // We clear the FB on each face draw call, not on Begin()
    _renderPolicy->_clearBuffersOnBind = false;

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM");
    shadowPreviewShader.setThreadedLoading(false);
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);
    _blurDepthMapShader->Uniform("texScreen", ShaderProgram::TextureUsage::UNIT0);

    Console::printfn(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(),
                     "EVCSM");
    SamplerDescriptor depthMapSampler;
    depthMapSampler.setFilters(TextureFilter::LINEAR_MIPMAP_LINEAR);
    depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    depthMapSampler.setAnisotropy(8);
    TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                         GFXImageFormat::RG32F,
                                         GFXDataFormat::FLOAT_32);
    depthMapDescriptor.setLayerCount(_numSplits);
    depthMapDescriptor.setSampler(depthMapSampler);

    _depthMap = GFX_DEVICE.newFB(false);
    _depthMap->AddAttachment(depthMapDescriptor,
                             TextureDescriptor::AttachmentType::Color0);
    _depthMap->toggleDepthBuffer(true);  //<create a floating point depth buffer
    _depthMap->setClearColor(DefaultColors::WHITE());

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TextureFilter::LINEAR);
    blurMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    depthMapSampler.setAnisotropy(0);
    TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                        GFXImageFormat::RG32F,
                                        GFXDataFormat::FLOAT_32);
    blurMapDescriptor.setLayerCount(_numSplits);
    blurMapDescriptor.setSampler(blurMapSampler);

    _blurBuffer = GFX_DEVICE.newFB(false);
    _blurBuffer->AddAttachment(blurMapDescriptor,
                               TextureDescriptor::AttachmentType::Color0);
    _blurBuffer->toggleDepthBuffer(false);
    _blurBuffer->setClearColor(DefaultColors::WHITE());
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    RemoveResource(_previewDepthMapShader);
    RemoveResource(_blurDepthMapShader);
    MemoryManager::DELETE(_blurBuffer);
    MemoryManager::DELETE(_renderPolicy);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi) {
    _numSplits = smi->numLayers();
    resolution(smi->resolution(), _light->shadowMapResolutionFactor());
    _init = true;
}

void CascadedShadowMaps::resolution(U16 resolution, U8 resolutionFactor) {
    U16 tempResolution = resolution * resolutionFactor;
    if (_resolution != tempResolution) {
        _resolution = tempResolution;
        // Initialize the FB's with a variable resolution
        Console::printfn(Locale::get("LIGHT_INIT_SHADOW_FB"),
                         _light->getGUID());
        _depthMap->Create(_resolution, _resolution);
        vec2<U16> screenResolution =
            GFX_DEVICE.getRenderTarget(
                           GFXDevice::RenderTarget::SCREEN)
                ->getResolution();
        _horizBlur = _blurDepthMapShader->GetSubroutineIndex(
            ShaderType::GEOMETRY, "computeCoordsH");
        _vertBlur = _blurDepthMapShader->GetSubroutineIndex(
            ShaderType::GEOMETRY, "computeCoordsV");
        _blurBuffer->Create(_resolution, _resolution);
        updateResolution(screenResolution.width, screenResolution.height);
    }

    ShadowMap::resolution(resolution, resolutionFactor);
}

void CascadedShadowMaps::updateResolution(I32 newWidth, I32 newHeight) {
    ShadowMap::updateResolution(newWidth, newHeight);
}

void CascadedShadowMaps::render(SceneRenderState& renderState,
                                const DELEGATE_CBK<>& sceneRenderFunction) {
    // Only if we have a valid callback;
    if (!sceneRenderFunction) {
        Console::errorfn(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"),
                         _light->getGUID());
        return;
    }

    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition = _light->getPosition();

    Camera& camera = renderState.getCamera();
    const vec2<F32>& currentPlanes = camera.getZPlanes();
    if (_sceneZPlanes != currentPlanes)
    {
        _sceneZPlanes = currentPlanes;
        CalculateSplitDepths(camera);
    }
    
    camera.getWorldMatrix(_viewInvMatrixCache);
    camera.getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera.getFrustum().getCornersViewSpace(_frustumCornersVS);

    _depthMap->Begin(*_renderPolicy);
        renderState.getCameraMgr().pushActiveCamera(_shadowCamera, false);
            for (U8 i = 0; i < _numSplits; ++i) {
                ApplyFrustumSplit(i);
                _depthMap->DrawToLayer(TextureDescriptor::AttachmentType::Color0, i,
                                       true);
                GFX_DEVICE.getRenderer().render(sceneRenderFunction, renderState);
            }
        renderState.getCameraMgr().popActiveCamera();
    _depthMap->End();
}

void CascadedShadowMaps::CalculateSplitDepths(const Camera& cam) {
    const mat4<F32>& projMatrixCache = cam.getProjectionMatrix();

    F32 N = _numSplits;
    F32 nearPlane = _sceneZPlanes.x;
    F32 farPlane = _sceneZPlanes.y;
    _splitDepths[0] = nearPlane;
    _splitDepths[_numSplits] = farPlane;
    for (I32 i = 1; i < (I32)_numSplits; ++i) {
        _splitDepths[i] = _splitLogFactor * nearPlane *
                              (F32)std::pow(farPlane / nearPlane, i / N) +
                          (1.0f - _splitLogFactor) *
                              ((nearPlane + (i / N)) * (farPlane - nearPlane));
    }

    for (U8 i = 0; i < _numSplits; ++i) {
        _light->setShadowFloatValue(
            i, 0.5f * (-_splitDepths[i + 1] * projMatrixCache.mat[10] +
                       projMatrixCache.mat[14]) /
                       _splitDepths[i + 1] +
                   0.5f);
    }
}

void CascadedShadowMaps::ApplyFrustumSplit(U8 pass) {
    F32 minZ = _splitDepths[pass];
    F32 maxZ = _splitDepths[pass + 1];

    for (U8 i = 0; i < 4; ++i) {
        _splitFrustumCornersVS[i] =
            _frustumCornersVS[i + 4] * (minZ / _sceneZPlanes.y);
    }

    for (U8 i = 4; i < 8; ++i) {
        _splitFrustumCornersVS[i] =
            _frustumCornersVS[i] * (maxZ / _sceneZPlanes.y);
    }

    for (U8 i = 0; i < 8; ++i) {
        _frustumCornersWS[i].set(
            _viewInvMatrixCache.transform(_splitFrustumCornersVS[i]));
    }

    vec3<F32> frustumCentroid(0.0f);

    for (U8 i = 0; i < 8; ++i) {
        frustumCentroid += _frustumCornersWS[i];
    }
    // Find the centroid
    frustumCentroid /= 8;

    // Position the shadow-caster camera so that it's looking at the centroid,
    // and backed up in the direction of the sunlight
    F32 distFromCentroid =
        std::max((maxZ - minZ),
                 _splitFrustumCornersVS[4].distance(_splitFrustumCornersVS[5]));
    // + _nearClipOffset;
    vec3<F32> currentEye =
        frustumCentroid - (_lightPosition * distFromCentroid);
    const mat4<F32>& viewMatrix =
        _shadowCamera->lookAt(currentEye, frustumCentroid);
    // Determine the position of the frustum corners in light space
    for (U8 i = 0; i < 8; ++i) {
        _frustumCornersLS[i].set(viewMatrix.transform(_frustumCornersWS[i]));
    }
    // Create an orthographic camera for use as a shadow caster
    vec2<F32> clipPlanes;
    vec4<F32> clipRect;

    if (_dirLight->csmStabilize()) {
        BoundingSphere frustumSphere(_frustumCornersLS);
        clipPlanes.set(0.0f, frustumSphere.getDiameter());
        clipPlanes += _nearClipOffset;
        clipRect.set(UNIT_RECT * frustumSphere.getRadius());
    } else {
        // Calculate an orthographic projection by sizing a bounding box to the
        // frustum coordinates in light space
        BoundingBox frustumBox(_frustumCornersLS);
        vec3<F32> maxes = frustumBox.getMax();
        vec3<F32> mins = frustumBox.getMin();
        // Create an orthographic camera for use as a shadow caster
        clipPlanes.set(-maxes.z - _nearClipOffset, -mins.z);
        clipRect.set(mins.x, maxes.x, mins.y, maxes.y);
    }

    _shadowCamera->setProjection(clipRect, clipPlanes, true);

    mat4<F32> lightViewProj = viewMatrix * _shadowCamera->getProjectionMatrix();

    // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
    F32 halfShadowMapSize = (_resolution)*0.5f;
    vec3<F32> testPoint =
        lightViewProj.transform(VECTOR3_ZERO) * halfShadowMapSize;
    vec3<F32> testPointRounded(testPoint);
    testPointRounded.round();
    vec3<F32> rounding = (testPointRounded - testPoint) / halfShadowMapSize;

    _light->setShadowVPMatrix(
        pass, mat4<F32>(rounding.x, rounding.y, 0.0f) * lightViewProj * _bias);
    _light->setShadowLightPos(pass, currentEye);
    _shadowCamera->renderLookAt();
}

void CascadedShadowMaps::postRender() {
    if (GFX_DEVICE.shadowDetailLevel() == RenderDetailLevel::LOW) {
        return;
    }

    GFX::Scoped2DRendering scoped2D(true);

    _blurDepthMapShader->bind();

    // Blur horizontally
    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _horizBlur);
    _blurDepthMapShader->Uniform("blurSize",
                                 1.0f / (_depthMap->getWidth() * 1.0f));
    _blurBuffer->Begin(*_renderPolicy);
    _depthMap->Bind();
    for (U8 i = 0; i < _numSplits - 1; ++i) {
        _blurDepthMapShader->Uniform("layer", (I32)i);
        _blurBuffer->DrawToLayer(TextureDescriptor::AttachmentType::Color0, i,
                                 false);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                              _blurDepthMapShader);
    }
    _blurBuffer->End();
    // Blur vertically
    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _vertBlur);
    _depthMap->Begin(*_renderPolicy);
    _blurBuffer->Bind();
    _blurDepthMapShader->Uniform("blurSize",
                                 1.0f / (_depthMap->getHeight() * 1.0f));
    for (U8 i = 0; i < _numSplits - 1; ++i) {
        _blurDepthMapShader->Uniform("layer", (I32)i);
        _depthMap->DrawToLayer(TextureDescriptor::AttachmentType::Color0, i,
                               false);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                              _blurDepthMapShader);
    }
    _depthMap->End();
}

bool CascadedShadowMaps::BindInternal(U8 offset) {
    _depthMap->Bind(offset);
    return true;
}

void CascadedShadowMaps::previewShadowMaps() {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }


    _depthMap->Bind();
    for (U8 i = 0; i < _numSplits; ++i) {
        _previewDepthMapShader->Uniform("layer", i);
        _previewDepthMapShader->Uniform(
            "dvd_zPlanes", vec2<F32>(_splitDepths[i], _splitDepths[i + 1]));

        GFX::ScopedViewport viewport(256 * i, 1, 256, 256);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true),
                              _previewDepthMapShader);
    }
}
};
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
                                       U8 numSplits)
    : ShadowMap(light, shadowCamera, ShadowType::LAYERED)
{
    _dirLight = dynamic_cast<DirectionalLight*>(_light);
    _splitLogFactor = 0.0f;
    _nearClipOffset = 0.0f;
    _numSplits = numSplits;
    _splitDepths.resize(_numSplits + 1);
    _frustumCornersWS.resize(8);
    _frustumCornersVS.resize(8);
    _frustumCornersLS.resize(8);
    _splitFrustumCornersVS.resize(8);
    _horizBlur = 0;
    _vertBlur = 0;
    _renderPolicy = MemoryManager_NEW Framebuffer::FramebufferTarget(
        Framebuffer::defaultPolicy());
    // We clear the FB on each face draw call, not on Begin()
    _renderPolicy->_clearBuffersOnBind = false;

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    shadowPreviewShader.setPropertyList("USE_SCENE_ZPLANES");
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);
    _blurDepthMapShader->Uniform("texScreen", ShaderProgram::TextureUsage::UNIT0);
    _blurDepthMapShader->Uniform("layerTotalCount", (I32)_numSplits);
    _blurDepthMapShader->Uniform("layerCount", (I32)_numSplits - 1);
    _blurDepthMapShader->Uniform("layerOffset", (I32)0);

    Console::printfn(Locale::get("LIGHT_CREATE_SHADOW_FB"), light->getGUID(), "EVCSM");

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
    _depthMap->addAttachment(depthMapDescriptor,
                             TextureDescriptor::AttachmentType::Color0);
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
    _blurBuffer->addAttachment(depthMapDescriptor,
                               TextureDescriptor::AttachmentType::Color0);
    _blurBuffer->setClearColor(DefaultColors::WHITE());

    _shadowMatricesBuffer.reset(GFX_DEVICE.newSB("dvd_shadowMatrices", 1, false, false, BufferUpdateFrequency::OFTEN));
    _shadowMatricesBuffer->create(Config::Lighting::MAX_SPLITS_PER_LIGHT, sizeof(mat4<F32>));
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
        Console::printfn(Locale::get("LIGHT_INIT_SHADOW_FB"), _light->getGUID());
        _depthMap->create(_resolution, _resolution);
        _blurBuffer->create(_resolution, _resolution);

        _blurDepthMapShader->bind();
        _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
        _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

        vectorImpl<vec2<F32>> blurSizes(_numSplits);
        blurSizes[0].set(1.0f / (_depthMap->getWidth() * 1.0f),
                         1.0f / (_depthMap->getHeight() * 1.0f));
        for (int i = 1; i < _numSplits; ++i) {
            blurSizes[i].set(blurSizes[i - 1] / 2.0f);
        }

        _blurDepthMapShader->Uniform("blurSizes", blurSizes);
    }

    ShadowMap::resolution(resolution, resolutionFactor);
}

void CascadedShadowMaps::render(SceneRenderState& renderState,
                                const DELEGATE_CBK<>& sceneRenderFunction) {
    // Only if we have a valid callback;
    if (!sceneRenderFunction) {
        Console::errorfn(Locale::get("ERROR_LIGHT_INVALID_SHADOW_CALLBACK"), _light->getGUID());
        return;
    }

    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition.set(_light->getPosition());

    Camera& camera = renderState.getCamera();
    _sceneZPlanes.set(camera.getZPlanes());
    camera.getWorldMatrix(_viewInvMatrixCache);
    camera.getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera.getFrustum().getCornersViewSpace(_frustumCornersVS);

    const vectorImpl<SceneGraphNode_wptr>& visibleNodes = SceneManager::getInstance().getVisibleShadowCasters();
    _previousFrustumBB.reset();
    for (SceneGraphNode_wptr node_wptr : visibleNodes) {
        SceneGraphNode_ptr node_ptr = node_wptr.lock();
        if (node_ptr) {
            _previousFrustumBB.add(node_ptr->getBoundingBoxConst());
        }
    }
    _previousFrustumBB.transform(_shadowCamera->getViewMatrix());

    calculateSplitDepths(camera);
    applyFrustumSplits();

    _shadowMatricesBuffer->setData(_shadowMatrices.data());
    _shadowMatricesBuffer->bind(ShaderBufferLocation::LIGHT_SHADOW);
    _shadowMatricesBuffer->incQueue();

    renderState.getCameraMgr().pushActiveCamera(_shadowCamera, false);
    _depthMap->begin(Framebuffer::defaultPolicy());
        GFX_DEVICE.getRenderer().render(sceneRenderFunction, renderState);
    _depthMap->end();
    renderState.getCameraMgr().popActiveCamera();
}

void CascadedShadowMaps::calculateSplitDepths(const Camera& cam) {
    const mat4<F32>& projMatrixCache = cam.getProjectionMatrix();

    F32 fd = _sceneZPlanes.y;//std::min(_sceneZPlanes.y, _previousFrustumBB.getExtent().z);
    F32 nd = _sceneZPlanes.x;//std::max(_sceneZPlanes.x, std::fabs(_previousFrustumBB.nearestDistanceFromPoint(cam.getEye())));

    F32 i_f = 1.0f, cascadeCount = (F32)_numSplits;
    _splitDepths[0] = nd;
    for (U32 i = 1; i < to_uint(_numSplits); ++i, i_f += 1.0f)
    {
        _splitDepths[i] = Lerp(nd + (i_f / cascadeCount)*(fd - nd),  nd * std::powf(fd / nd, i_f / cascadeCount), _splitLogFactor);
    }
    _splitDepths[_numSplits] = fd;

    for (U8 i = 0; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        if (i < _numSplits) {
            _light->setShadowFloatValue(i, vec4<F32>(0.5f * (-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f));
        } else {
            _light->setShadowFloatValue(i, vec4<F32>(1.0f));
        }
    }
}

void CascadedShadowMaps::applyFrustumSplits() {
    for (U8 pass = 0 ; pass < _numSplits; ++pass) {
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
        F32 distFromCentroid = std::max((maxZ - minZ),
                     _splitFrustumCornersVS[4].distance(_splitFrustumCornersVS[5])) + _nearClipOffset;
    
        vec3<F32> currentEye = frustumCentroid - (_lightPosition * distFromCentroid);

        const mat4<F32>& viewMatrix = _shadowCamera->lookAt(currentEye, frustumCentroid);
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
            BoundingBox frustumBox(_frustumCornersLS);
            // Calculate an orthographic projection by sizing a bounding box to the
            // frustum coordinates in light space
            vec3<F32> maxes = frustumBox.getMax();
            vec3<F32> mins = frustumBox.getMin();
            // Create an orthographic camera for use as a shadow caster
            clipPlanes.set(-maxes.z - _nearClipOffset, -mins.z);
            clipRect.set(mins.x, maxes.x, mins.y, maxes.y);
        }

        _shadowCamera->setProjection(clipRect, clipPlanes, true);

        mat4<F32>& currentPassMat = _shadowMatrices[pass];
        currentPassMat.set(viewMatrix * _shadowCamera->getProjectionMatrix());

        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = (_resolution)*0.5f;
        vec3<F32> testPoint = _shadowMatrices[pass].transform(VECTOR3_ZERO) * halfShadowMapSize;
        vec3<F32> testPointRounded(testPoint);
        testPointRounded.round();
        vec3<F32> rounding = (testPointRounded - testPoint) / halfShadowMapSize;

        _light->setShadowVPMatrix(pass, mat4<F32>(rounding.x, rounding.y, 0.0f) * _shadowMatrices[pass] * _bias);
        _light->setShadowLightPos(pass, currentEye);
    }
}

void CascadedShadowMaps::postRender() {
    if (GFX_DEVICE.shadowDetailLevel() == RenderDetailLevel::LOW) {
        return;
    }
    _blurDepthMapShader->bind();

    // Blur horizontally
    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _horizBlur);
    _blurBuffer->begin(Framebuffer::defaultPolicy());
    _depthMap->bind(0, TextureDescriptor::AttachmentType::Color0, false);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _blurDepthMapShader);
    _blurBuffer->end();

    // Blur vertically
    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _vertBlur);
    _depthMap->begin(Framebuffer::defaultPolicy());
    _blurBuffer->bind();
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _blurDepthMapShader);
    _depthMap->end();
}

bool CascadedShadowMaps::bindInternal(U8 offset) {
    _depthMap->bind(offset);
    return true;
}

void CascadedShadowMaps::previewShadowMaps() {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    size_t stateHash = GFX_DEVICE.getDefaultStateBlock(true);
    _depthMap->bind();
    for (U32 i = 0; i < _numSplits; ++i) {
        _previewDepthMapShader->Uniform("layer", i);
        _previewDepthMapShader->Uniform("dvd_zPlanes", _splitDepths[i]);

        GFX::ScopedViewport viewport(256 * i, 1, 256, 256);
        GFX_DEVICE.drawTriangle(stateHash, _previewDepthMapShader);
    }
}
};
#include "Headers/CascadedShadowMaps.h"

#include "Core/Headers/ParamHandler.h"

#include "Scenes/Headers/SceneState.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/LightManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderManager.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

CascadedShadowMaps::CascadedShadowMaps(Light* light, Camera* shadowCamera, U8 numSplits)
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
    _renderPolicy = MemoryManager_NEW Framebuffer::FramebufferTarget(Framebuffer::defaultPolicy());
    // We clear the FB on each face draw call, not on Begin()
    _renderPolicy->_clearDepthBufferOnBind = false;
    _renderPolicy->_clearColorBuffersOnBind = false;

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    shadowPreviewShader.setPropertyList("USE_SCENE_ZPLANES");
    _previewDepthMapShader = CreateResource<ShaderProgram>(shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(blurDepthMapShader);
    _blurDepthMapShader->Uniform("layerTotalCount", (I32)_numSplits);
    _blurDepthMapShader->Uniform("layerCount", (I32)_numSplits - 1);
    _blurDepthMapShader->Uniform("layerOffset", (I32)getArrayOffset());

    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "EVCSM");

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TextureFilter::LINEAR);
    blurMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    blurMapSampler.setAnisotropy(0);
    blurMapSampler.toggleMipMaps(false);
    TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                        GFXImageFormat::RG32F,
                                        GFXDataFormat::FLOAT_32);
    blurMapDescriptor.setLayerCount(_numSplits);
    blurMapDescriptor.setSampler(blurMapSampler);
    
    _blurBuffer = GFX_DEVICE.newFB(false);
    _blurBuffer->addAttachment(blurMapDescriptor,
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
    Framebuffer* depthMap = getDepthMap();
    _blurBuffer->create(depthMap->getWidth(), depthMap->getHeight());
    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    vectorImpl<vec2<F32>> blurSizes(_numSplits);
    blurSizes[0].set(1.0f / (depthMap->getWidth() * 1.0f), 1.0f / (depthMap->getHeight() * 1.0f));
    for (int i = 1; i < _numSplits; ++i) {
        blurSizes[i].set(blurSizes[i - 1] / 2.0f);
    }

    _blurDepthMapShader->Uniform("blurSizes", blurSizes);

    _init = true;
}

void CascadedShadowMaps::render(SceneRenderState& renderState) {
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition.set(_light->getPosition());

    Camera& camera = renderState.getCamera();
    _sceneZPlanes.set(camera.getZPlanes());
    camera.getWorldMatrix(_viewInvMatrixCache);
    camera.getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera.getFrustum().getCornersViewSpace(_frustumCornersVS);

    calculateSplitDepths(camera);
    applyFrustumSplits();

    /*const RenderPassCuller::VisibleNodeList& nodes = 
        SceneManager::getInstance().cullSceneGraph(RenderStage::SHADOW);

    _previousFrustumBB.reset();
    for (SceneGraphNode_wptr node_wptr : nodes) {
        SceneGraphNode_ptr node_ptr = node_wptr.lock();
        if (node_ptr) {
            _previousFrustumBB.add(node_ptr->getBoundingBoxConst());
        }
    }
    _previousFrustumBB.transformHomogeneous(_shadowCamera->getViewMatrix());*/

    _shadowMatricesBuffer->setData(_shadowMatrices.data());
    _shadowMatricesBuffer->bind(ShaderBufferLocation::LIGHT_SHADOW);
    _shadowMatricesBuffer->incQueue();

    renderState.getCameraMgr().pushActiveCamera(_shadowCamera);
    getDepthMap()->begin(*_renderPolicy);
        SceneManager::getInstance().renderVisibleNodes(RenderStage::SHADOW, true);
    getDepthMap()->end();
    renderState.getCameraMgr().popActiveCamera();

    postRender();
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
            _light->setShadowFloatValue(i, 0.5f * (-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f);
        } else {
            _light->setShadowFloatValue(i, 1.0f);
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
                _viewInvMatrixCache.transformHomogeneous(_splitFrustumCornersVS[i]));
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
            _frustumCornersLS[i].set(viewMatrix.transformHomogeneous(_frustumCornersWS[i]));
        }

        F32 frustumSphereRadius = BoundingSphere(_frustumCornersLS).getRadius();
        vec2<F32> clipPlanes(std::max(0.0f, minZ - _nearClipOffset), (frustumSphereRadius * 2) + _nearClipOffset);
        _shadowCamera->setProjection(UNIT_RECT * frustumSphereRadius,
                                     clipPlanes,
                                     true);
        _shadowMatrices[pass].set(viewMatrix * _shadowCamera->getProjectionMatrix());

        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = (getDepthMap()->getWidth())*0.5f;
        vec3<F32> testPoint = _shadowMatrices[pass].transformHomogeneous(VECTOR3_ZERO) * halfShadowMapSize;
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
    getDepthMap()->bind(0, TextureDescriptor::AttachmentType::Color0, false);
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _blurDepthMapShader);
    getDepthMap()->end();

    // Blur vertically
    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _vertBlur);
    getDepthMap()->begin(Framebuffer::defaultPolicy());
    _blurBuffer->bind();
        GFX_DEVICE.drawPoints(1, GFX_DEVICE.getDefaultStateBlock(true), _blurDepthMapShader);
    getDepthMap()->end();
}

void CascadedShadowMaps::previewShadowMaps(U32 rowIndex) {
    if (_previewDepthMapShader->getState() != ResourceState::RES_LOADED) {
        return;
    }

    const vec4<I32> viewport = getViewportForRow(rowIndex);

    U32 stateHash = GFX_DEVICE.getDefaultStateBlock(true);
    getDepthMap()->bind();
    for (U32 i = 0; i < _numSplits; ++i) {
        _previewDepthMapShader->Uniform("layer", i + _arrayOffset);

        GFX::ScopedViewport sViewport(viewport.x * i, viewport.y, viewport.z, viewport.w);
        GFX_DEVICE.drawTriangle(stateHash, _previewDepthMapShader);
    }
}
};
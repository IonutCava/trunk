#include "Headers/CascadedShadowMaps.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

CascadedShadowMaps::CascadedShadowMaps(GFXDevice& context, Light* light, Camera* shadowCamera, U8 numSplits)
    : ShadowMap(context, light, shadowCamera, ShadowType::LAYERED)
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

    ResourceDescriptor shadowPreviewShader("fbPreview.Layered.LinearDepth.ESM.ScenePlanes");
    shadowPreviewShader.setThreadedLoading(false);
    shadowPreviewShader.setPropertyList("USE_SCENE_ZPLANES");
    _previewDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), shadowPreviewShader);
    _previewDepthMapShader->Uniform("useScenePlanes", false);

    for (U32 i = 0; i < _numSplits; ++i) {
        GFXDevice::DebugView_ptr shadow = std::make_shared<GFXDevice::DebugView>();
        shadow->_texture = getDepthMap().getAttachment(RTAttachment::Type::Colour, 0).asTexture();
        shadow->_shader = _previewDepthMapShader;
        shadow->_shaderData._intValues.push_back(std::make_pair("layer", i + _arrayOffset));
        _context.addDebugView(shadow);
    }

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), blurDepthMapShader);
    _blurDepthMapShader->Uniform("layerTotalCount", (I32)_numSplits);
    _blurDepthMapShader->Uniform("layerCount",  (I32)_numSplits);

    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "EVCSM");

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TextureFilter::LINEAR);
    blurMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    blurMapSampler.setAnisotropy(0);
    blurMapSampler.toggleMipMaps(false);
    TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                        GFXImageFormat::RG32F,
                                        GFXDataFormat::FLOAT_32);
    blurMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
    blurMapDescriptor.setSampler(blurMapSampler);
    
    _blurBuffer = _context.allocateRT("CSM_Blur");
    _blurBuffer._rt->addAttachment(blurMapDescriptor, RTAttachment::Type::Colour, 0);
    _blurBuffer._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());

    ShaderBufferParams params;
    params._primitiveCount = Config::Lighting::MAX_SPLITS_PER_LIGHT;
    params._primitiveSizeInBytes = sizeof(mat4<F32>);
    params._ringBufferLength = 1;
    params._unbound = false;
    params._updateFrequency = BufferUpdateFrequency::OFTEN;

    _shadowMatricesBuffer = _context.newSB(params);

    STUBBED("Migrate to this: http://www.ogldev.org/www/tutorial49/tutorial49.html");
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    _context.deallocateRT(_blurBuffer);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi) {
    _numSplits = smi->numLayers();
    const RenderTarget& depthMap = getDepthMap();
    _blurBuffer._rt->create(depthMap.getWidth(), depthMap.getHeight());
    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    vectorImpl<vec2<F32>> blurSizes(_numSplits);
    blurSizes[0].set(1.0f / (depthMap.getWidth() * 1.0f), 1.0f / (depthMap.getHeight() * 1.0f));
    for (int i = 1; i < _numSplits; ++i) {
        blurSizes[i].set(blurSizes[i - 1] / 2.0f);
    }

    _blurDepthMapShader->Uniform("blurSizes", blurSizes);

    _init = true;
}

void CascadedShadowMaps::render(GFXDevice& context, U32 passIdx) {
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition.set(_light->getPosition());

    Camera& camera = *Camera::activeCamera();
    _sceneZPlanes.set(camera.getZPlanes());
    camera.getWorldMatrix(_viewInvMatrixCache);
    camera.getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera.getFrustum().getCornersViewSpace(_frustumCornersVS);

    calculateSplitDepths(camera);
    applyFrustumSplits();

    /*const RenderPassCuller::VisibleNodeList& nodes = 
        SceneManager::instance().cullSceneGraph(RenderStage::SHADOW);

    _previousFrustumBB.reset();
    for (RenderPassCuller::VisibleNode node : nodes) {
        SceneGraphNode* node_ptr = node.second;
        if (node_ptr) {
            _previousFrustumBB.add(node_ptr->getBoundingBoxConst());
        }
    }
    _previousFrustumBB.transformHomogeneous(_shadowCamera->getViewMatrix());*/

    _shadowMatricesBuffer->setData(_shadowMatrices.data());
    _shadowMatricesBuffer->bind(ShaderBufferLocation::LIGHT_SHADOW);
    _shadowMatricesBuffer->incQueue();

    RenderPassManager::PassParams params;
    params.doPrePass = false;
    params.camera = _shadowCamera;
    params.stage = RenderStage::SHADOW;
    params.target = RenderTargetID(RenderTargetUsage::SHADOW, to_U32(getShadowMapType()));
    params.pass = passIdx;

    context.parent().renderPassManager().doCustomPass(params);

    postRender(context);
}

void CascadedShadowMaps::calculateSplitDepths(const Camera& cam) {
    const mat4<F32>& projMatrixCache = cam.getProjectionMatrix();

    F32 fd = _sceneZPlanes.y;//std::min(_sceneZPlanes.y, _previousFrustumBB.getExtent().z);
    F32 nd = _sceneZPlanes.x;//std::max(_sceneZPlanes.x, std::fabs(_previousFrustumBB.nearestDistanceFromPoint(cam.getEye())));

    F32 i_f = 1.0f, cascadeCount = (F32)_numSplits;
    _splitDepths[0] = nd;
    for (U32 i = 1; i < to_U32(_numSplits); ++i, i_f += 1.0f)
    {
        _splitDepths[i] = Lerp(nd + (i_f / cascadeCount)*(fd - nd),  nd * std::powf(fd / nd, i_f / cascadeCount), _splitLogFactor);
    }
    _splitDepths[_numSplits] = fd;

    for (U8 i = 0; i < _numSplits; ++i) {
        F32 crtSplitDepth = 0.5f * (-_splitDepths[i + 1] * projMatrixCache.mat[10] + projMatrixCache.mat[14]) / _splitDepths[i + 1] + 0.5f;
       _light->setShadowFloatValue(i, crtSplitDepth);
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

        const mat4<F32>& viewMatrix = _shadowCamera->lookAt(currentEye, frustumCentroid - currentEye);
        // Determine the position of the frustum corners in light space
        for (U8 i = 0; i < 8; ++i) {
            _frustumCornersLS[i].set(viewMatrix.transformHomogeneous(_frustumCornersWS[i]));
        }

        F32 frustumSphereRadius = BoundingSphere(_frustumCornersLS).getRadius();
        vec2<F32> clipPlanes(std::max(1.0f, minZ - _nearClipOffset), frustumSphereRadius * 2 + _nearClipOffset * 2);
        const mat4<F32>& projMatrix = _shadowCamera->setProjection(UNIT_RECT * frustumSphereRadius, clipPlanes);
        mat4<F32>::Multiply(viewMatrix, projMatrix, _shadowMatrices[pass]);

        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = getDepthMap().getWidth() * 0.5f;
        vec3<F32> testPoint = _shadowMatrices[pass].transformHomogeneous(VECTOR3_ZERO) * halfShadowMapSize;
        vec3<F32> testPointRounded(testPoint);
        testPointRounded.round();
        _shadowMatrices[pass].translate(vec3<F32>(((testPointRounded - testPoint) / halfShadowMapSize).xy(), 0.0f));

        _light->setShadowVPMatrix(pass, _shadowMatrices[pass] * MAT4_BIAS);
        _light->setShadowLightPos(pass, currentEye);
    }
}

void CascadedShadowMaps::postRender(GFXDevice& context) {
    if (context.shadowDetailLevel() == RenderDetailLevel::LOW) {
        return;
    }

    RenderTarget& depthMap = getDepthMap();

    _blurDepthMapShader->bind();

    GenericDrawCommand pointsCmd;
    pointsCmd.primitiveType(PrimitiveType::API_POINTS);
    pointsCmd.drawCount(1);
    pointsCmd.stateHash(context.getDefaultStateBlock(true));
    pointsCmd.shaderProgram(_blurDepthMapShader);

    // Blur horizontally
    _blurDepthMapShader->Uniform("layerOffsetRead", (I32)getArrayOffset());
    _blurDepthMapShader->Uniform("layerOffsetWrite", 0);

    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _horizBlur);
    _blurBuffer._rt->begin(RenderTarget::defaultPolicy());
    depthMap.bind(0, RTAttachment::Type::Colour, 0, false);
        context.draw(pointsCmd);
    depthMap.end();

    // Blur vertically
    _blurDepthMapShader->Uniform("layerOffsetRead", 0);
    _blurDepthMapShader->Uniform("layerOffsetWrite", (I32)getArrayOffset());

    _blurDepthMapShader->SetSubroutine(ShaderType::GEOMETRY, _vertBlur);
    depthMap.begin(RenderTarget::defaultPolicy());
    _blurBuffer._rt->bind(0, RTAttachment::Type::Colour, 0);
        context.draw(pointsCmd);
    depthMap.end();
}

};
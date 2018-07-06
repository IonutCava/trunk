#include "stdafx.h"

#include "Headers/CascadedShadowMaps.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

CascadedShadowMaps::CascadedShadowMaps(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras, U8 numSplits)
    : ShadowMap(context, light, shadowCameras, ShadowType::LAYERED)
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
    for (U32 i = 0; i < _numSplits; ++i) {
        GFXDevice::DebugView_ptr shadow = std::make_shared<GFXDevice::DebugView>();
        shadow->_texture = getDepthMap().getAttachment(RTAttachmentType::Colour, 0).texture();
        shadow->_shader = _previewDepthMapShader;
        shadow->_shaderData.set("layer", GFX::PushConstantType::INT, i+ _arrayOffset);
        shadow->_shaderData.set("useScenePlanes", GFX::PushConstantType::BOOL, false);
        shadow->_name = Util::StringFormat("CSM_%d", i + _arrayOffset);
        _context.addDebugView(shadow);
    }

    ResourceDescriptor blurDepthMapShader("blur.GaussBlur");
    blurDepthMapShader.setThreadedLoading(false);
    _blurDepthMapShader = CreateResource<ShaderProgram>(light->parentResourceCache(), blurDepthMapShader);
    _blurDepthMapConstants.set("layerTotalCount", GFX::PushConstantType::INT, (I32)_numSplits);
    _blurDepthMapConstants.set("layerCount", GFX::PushConstantType::INT, (I32)_numSplits);
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), light->getGUID(), "EVCSM");

    SamplerDescriptor blurMapSampler;
    blurMapSampler.setFilters(TextureFilter::LINEAR);
    blurMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    blurMapSampler.setAnisotropy(0);

    TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                        GFXImageFormat::RG32F,
                                        GFXDataFormat::FLOAT_32);
    blurMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
    blurMapDescriptor.setSampler(blurMapSampler);
    
    const RenderTarget& depthMap = getDepthMap();

    vector<RTAttachmentDescriptor> att = {
        { blurMapDescriptor, RTAttachmentType::Colour },
    };

    RenderTargetDescriptor desc = {};
    desc._name = "CSM_Blur";
    desc._resolution = vec2<U16>(depthMap.getWidth(), depthMap.getHeight());
    desc._attachmentCount = to_U8(att.size());
    desc._attachments = att.data();

    _blurBuffer = _context.renderTargetPool().allocateRT(desc);

    STUBBED("Migrate to this: http://www.ogldev.org/www/tutorial49/tutorial49.html");
}

CascadedShadowMaps::~CascadedShadowMaps()
{
    _context.renderTargetPool().deallocateRT(_blurBuffer);
}

void CascadedShadowMaps::init(ShadowMapInfo* const smi) {
    _numSplits = smi->numLayers();
    const RenderTarget& depthMap = getDepthMap();

    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    vector<vec2<F32>> blurSizes(_numSplits);
    blurSizes[0].set(1.0f / (depthMap.getWidth() * 1.0f), 1.0f / (depthMap.getHeight() * 1.0f));
    for (int i = 1; i < _numSplits; ++i) {
        blurSizes[i].set(blurSizes[i - 1] / 2.0f);
    }

    _blurDepthMapConstants.set("blurSizes", GFX::PushConstantType::VEC2, blurSizes);
    _init = true;
}

void CascadedShadowMaps::render(U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    _splitLogFactor = _dirLight->csmSplitLogFactor();
    _nearClipOffset = _dirLight->csmNearClipOffset();
    _lightPosition.set(_light->getPosition());

    Camera* camera = playerCamera();
    _sceneZPlanes.set(camera->getZPlanes());
    camera->getWorldMatrix(_viewInvMatrixCache);
    camera->getFrustum().getCornersWorldSpace(_frustumCornersWS);
    camera->getFrustum().getCornersViewSpace(_frustumCornersVS);

    calculateSplitDepths(*camera);
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

    RenderPassManager::PassParams params;
    params._doPrePass = false;
    params._stage = RenderStage::SHADOW;
    params._target = RenderTargetID(RenderTargetUsage::SHADOW, to_U32(getShadowMapType()));
    params._pass = passIdx;
    params._bindTargets = false;

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = params._target;
    beginRenderPassCmd._name = "DO_CSM_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
    GFX::EndRenderSubPassCommand endRenderSubPassCommand;

    GFX::BeginDebugScopeCommand beginDebugScopeCommand;
    GFX::EndDebugScopeCommand endDebugScopeCommand;

    RenderTarget::DrawLayerParams drawParams;
    drawParams._type = RTAttachmentType::Colour;
    drawParams._index = 0;
    
    RenderPassManager& rpm = _context.parent().renderPassManager();
    for (U8 i = 0; i < _numSplits; ++i) {
        beginDebugScopeCommand._scopeName = Util::StringFormat("CSM_PASS_%d", i).c_str();
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCommand);

        drawParams._layer = to_U16(i + getArrayOffset());
        beginRenderSubPassCmd._writeLayers.resize(1, drawParams);
        GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);

        params._pass += i;
        params._camera = _shadowCameras[i];
        rpm.doCustomPass(params, bufferInOut);

        GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }
    
    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    postRender(bufferInOut);
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

        const mat4<F32>& viewMatrix = _shadowCameras[pass]->lookAt(currentEye, Normalize(frustumCentroid - currentEye));
        // Determine the position of the frustum corners in light space
        for (U8 i = 0; i < 8; ++i) {
            _frustumCornersLS[i].set(viewMatrix.transformHomogeneous(_frustumCornersWS[i]));
        }

        F32 frustumSphereRadius = BoundingSphere(_frustumCornersLS).getRadius();
        vec2<F32> clipPlanes(std::max(1.0f, minZ - _nearClipOffset), frustumSphereRadius * 2 + _nearClipOffset * 2);
        const mat4<F32>& projMatrix = _shadowCameras[pass]->setProjection(UNIT_RECT * frustumSphereRadius, clipPlanes);
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

void CascadedShadowMaps::postRender(GFX::CommandBuffer& bufferInOut) {
    if (_context.shadowDetailLevel() == RenderDetailLevel::LOW) {
        return;
    }

    RenderTarget& depthMap = getDepthMap();

    PipelineDescriptor pipelineDescriptor;
    pipelineDescriptor._stateHash = _context.get2DStateBlock();
    pipelineDescriptor._shaderProgramHandle = _blurDepthMapShader->getID();
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].push_back(_horizBlur);

    GenericDrawCommand pointsCmd;
    pointsCmd._primitiveType = PrimitiveType::API_POINTS;
    pointsCmd._drawCount = 1;
    
    GFX::BindPipelineCommand pipelineCmd;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    // Blur horizontally
    _blurDepthMapConstants.set("layerOffsetRead", GFX::PushConstantType::INT, (I32)getArrayOffset());
    _blurDepthMapConstants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)0);

    GFX::SendPushConstantsCommand pushConstantsCommand;
    pushConstantsCommand._constants = _blurDepthMapConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

    TextureData texData = depthMap.getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = _context.newDescriptorSet(); 
    descriptorSetCmd._set->_textureData.addTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = _blurBuffer._targetID;
    beginRenderPassCmd._name = "DO_CSM_BLUR_PASS";
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::DrawCommand drawCmd;
    drawCmd._drawCommands.push_back(pointsCmd);
    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    // Blur vertically
    pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].front() = _vertBlur;
    pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
    GFX::EnqueueCommand(bufferInOut, pipelineCmd);

    _blurDepthMapConstants.set("layerOffsetRead", GFX::PushConstantType::INT, (I32)0);
    _blurDepthMapConstants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)getArrayOffset());

    pushConstantsCommand._constants = _blurDepthMapConstants;
    GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);
    
    texData = _blurBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
    descriptorSetCmd._set = _context.newDescriptorSet();
    descriptorSetCmd._set->_textureData.addTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

    beginRenderPassCmd._target = getDepthMapID();
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EnqueueCommand(bufferInOut, drawCmd);

    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
}

};
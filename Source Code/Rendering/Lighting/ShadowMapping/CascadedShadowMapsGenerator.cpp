#include "stdafx.h"

#include "Headers/CascadedShadowMapsGenerator.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/Light.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/StringHelper.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Shaders/Headers/ShaderProgram.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

namespace Divide {

namespace{
    Configuration::Rendering::ShadowMapping g_shadowSettings;
};

CascadedShadowMapsGenerator::CascadedShadowMapsGenerator(GFXDevice& context)
    : ShadowMapGenerator(context)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "EVCSM");

    g_shadowSettings = context.parent().platformContext().config().rendering.shadowMapping;
    ResourceDescriptor blurDepthMapShader(Util::StringFormat("blur.GaussBlur_%d_invocations", Config::Lighting::MAX_SPLITS_PER_LIGHT));
    blurDepthMapShader.setThreadedLoading(false);
    blurDepthMapShader.setPropertyList(Util::StringFormat("GS_MAX_INVOCATIONS %d", Config::Lighting::MAX_SPLITS_PER_LIGHT));

    _blurDepthMapShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), blurDepthMapShader);
    _blurDepthMapConstants.set("layerCount", GFX::PushConstantType::INT, Config::Lighting::MAX_SPLITS_PER_LIGHT);
    _blurDepthMapConstants.set("layerOffsetRead", GFX::PushConstantType::INT, (I32)0);
    _blurDepthMapConstants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)0);
    _horizBlur = 0;
    _vertBlur = 0;

    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    vector<vec2<F32>> blurSizes(Config::Lighting::MAX_SPLITS_PER_LIGHT);
    blurSizes[0].set(1.0f / g_shadowSettings.shadowMapResolution);
    for (U8 i = 1; i < Config::Lighting::MAX_SPLITS_PER_LIGHT; ++i) {
        blurSizes[i].set(blurSizes[i - 1] * 0.5f);
    }

    _blurDepthMapConstants.set("blurSizes", GFX::PushConstantType::VEC2, blurSizes);

    SamplerDescriptor sampler;
    sampler.setFilters(TextureFilter::LINEAR);
    sampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
    sampler.setAnisotropy(0);

    // Draw FBO
    {
        // MSAA rendering is supported
        TextureType texType = g_shadowSettings.msaaSamples > 0 ? TextureType::TEXTURE_2D_ARRAY_MS : TextureType::TEXTURE_2D_ARRAY;
        TextureDescriptor depthMapDescriptor(texType, GFXImageFormat::RG32F, GFXDataFormat::FLOAT_32);
        depthMapDescriptor.msaaSamples(g_shadowSettings.msaaSamples);
        depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
        depthMapDescriptor.setSampler(sampler);

        TextureDescriptor depthDescriptor(texType, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);
        depthDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
        depthDescriptor.setSampler(sampler);
        depthDescriptor.msaaSamples(g_shadowSettings.msaaSamples);

        vector<RTAttachmentDescriptor> att = {
            { depthMapDescriptor, RTAttachmentType::Colour },
            { depthDescriptor, RTAttachmentType::Depth },
        };

        RenderTargetDescriptor desc = {};
        desc._name = "CSM_ShadowMap_Draw";
        desc._resolution = vec2<U16>(g_shadowSettings.shadowMapResolution);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _drawBuffer = context.renderTargetPool().allocateRT(desc);
    }

    //Blur FBO
    {
        TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY, GFXImageFormat::RG32F, GFXDataFormat::FLOAT_32);
        blurMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT);
        blurMapDescriptor.setSampler(sampler);

        vector<RTAttachmentDescriptor> att = {
            { blurMapDescriptor, RTAttachmentType::Colour }
        };

        RenderTargetDescriptor desc = {};
        desc._name = "CSM_Blur";
        desc._resolution = vec2<U16>(g_shadowSettings.shadowMapResolution);
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _blurBuffer = _context.renderTargetPool().allocateRT(desc);
    }

    STUBBED("Migrate to this: http://www.ogldev.org/www/tutorial49/tutorial49.html");
}

CascadedShadowMapsGenerator::~CascadedShadowMapsGenerator()
{
    _context.renderTargetPool().deallocateRT(_blurBuffer);
    _context.renderTargetPool().deallocateRT(_drawBuffer);
}

void CascadedShadowMapsGenerator::render(const Camera& playerCamera, Light& light, U32 passIdx, GFX::CommandBuffer& bufferInOut) {
    std::array<vec3<F32>, 8> frustumCornersVS;
    std::array<vec3<F32>, 8> frustumCornersWS;

    DirectionalLight& dirLight = static_cast<DirectionalLight&>(light);

    U8 numSplits = dirLight.csmSplitCount();
    playerCamera.getFrustum().getCornersWorldSpace(frustumCornersWS);
    playerCamera.getFrustum().getCornersViewSpace(frustumCornersVS);

    const vec2<F32>& nearFarPlanes = playerCamera.getZPlanes();
    SplitDepths splitDepths = calculateSplitDepths(playerCamera.getProjectionMatrix(), dirLight, nearFarPlanes);
    applyFrustumSplits(dirLight, playerCamera.getWorldMatrix(), numSplits, splitDepths, nearFarPlanes, frustumCornersWS, frustumCornersVS);
    
    RenderPassManager::PassParams params;
    params._doPrePass = false;
    params._stage = RenderStage::SHADOW;
    params._target = _drawBuffer._targetID;
    params._bindTargets = false;

    GFX::BeginRenderPassCommand beginRenderPassCmd;
    beginRenderPassCmd._target = params._target;
    beginRenderPassCmd._name = "DO_CSM_PASS";
    beginRenderPassCmd._descriptor = RenderTarget::defaultPolicy();
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EndRenderSubPassCommand endRenderSubPassCommand;

    GFX::BeginDebugScopeCommand beginDebugScopeCommand;
    GFX::EndDebugScopeCommand endDebugScopeCommand;

    RenderTarget::DrawLayerParams drawParams;
    drawParams._type = RTAttachmentType::Colour;
    drawParams._index = 0;
    drawParams._layer = 0;

    RenderPassManager& rpm = _context.parent().renderPassManager();
    for (U8 i = 0; i < numSplits; ++i) {
        beginDebugScopeCommand._scopeName = Util::StringFormat("CSM_PASS_%d", i).c_str();
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCommand);

        GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
        beginRenderSubPassCmd._writeLayers.push_back(drawParams);
        GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);

        params._passIndex = passIdx;
        params._bufferIndex = i;
        params._camera = light.shadowCameras()[i];
        rpm.doCustomPass(params, bufferInOut);

        GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);

        ++drawParams._layer;

        //_debugViews[i]->_shaderData.set("zPlanes", GFX::PushConstantType::VEC2, _shadowCameras[i]->getZPlanes());
    }
    
    GFX::BeginRenderSubPassCommand beginRenderSubPassCmd;
    drawParams._layer = 0;
    beginRenderSubPassCmd._writeLayers.push_back(drawParams);
    GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);
    GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);

    GFX::EndRenderPassCommand endRenderPassCmd;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    postRender(dirLight, bufferInOut);
}

CascadedShadowMapsGenerator::SplitDepths CascadedShadowMapsGenerator::calculateSplitDepths(const mat4<F32>& projMatrix, DirectionalLight& light, const vec2<F32>& nearFarPlanes) {
    SplitDepths depths;

    F32 fd = nearFarPlanes.y;//std::min(_sceneZPlanes.y, _previousFrustumBB.getExtent().z);
    F32 nd = nearFarPlanes.x;//std::max(_sceneZPlanes.x, std::fabs(_previousFrustumBB.nearestDistanceFromPoint(cam.getEye())));

    U8 numSplits = light.csmSplitCount();
    F32 splitLogFactor = light.csmSplitLogFactor();


    F32 i_f = 1.0f, cascadeCount = (F32)numSplits;
    depths[0] = nd;
    for (U32 i = 1; i < to_U32(numSplits); ++i, i_f += 1.0f)
    {
        depths[i] = Lerp(nd + (i_f / cascadeCount)*(fd - nd),  nd * std::powf(fd / nd, i_f / cascadeCount), splitLogFactor);
    }
    depths[numSplits] = fd;

    for (U8 i = 0; i < numSplits; ++i) {
        F32 crtSplitDepth = 0.5f * (-depths[i + 1] * projMatrix.mat[10] + projMatrix.mat[14]) / depths[i + 1] + 0.5f;
       light.setShadowFloatValue(i, crtSplitDepth);
    }

    return depths;
}

void CascadedShadowMapsGenerator::applyFrustumSplits(DirectionalLight& light,
                                                     const mat4<F32>& invViewMatrix,
                                                     U8 numSplits,
                                                     const SplitDepths& splitDepths,
                                                     const vec2<F32>& nearFarPlanes,
                                                     std::array<vec3<F32>, 8>& frustumWS,
                                                     std::array<vec3<F32>, 8>& frustumVS)
{
    std::array<vec3<F32>, 8> splitFrustumCornersVS;
    std::array<vec3<F32>, 8> frustumCornersLS;

    F32 nearClipOffset = light.csmNearClipOffset();

    for (U8 pass = 0 ; pass < numSplits; ++pass) {
        F32 minZ = splitDepths[pass];
        F32 maxZ = splitDepths[pass + 1];

        for (U8 i = 0; i < 4; ++i) {
            splitFrustumCornersVS[i] = frustumVS[i + 4] * (minZ / nearFarPlanes.y);
        }

        for (U8 i = 4; i < 8; ++i) {
            splitFrustumCornersVS[i] = frustumVS[i] * (maxZ / nearFarPlanes.y);
        }

        for (U8 i = 0; i < 8; ++i) {
            frustumWS[i].set(invViewMatrix.transformHomogeneous(splitFrustumCornersVS[i]));
        }

        vec3<F32> frustumCentroid = frustumWS[0];

        for (U8 i = 1; i < 8; ++i) {
            frustumCentroid += frustumWS[i];
        }
        // Find the centroid
        frustumCentroid /= 8;

        // Position the shadow-caster camera so that it's looking at the centroid,
        // and backed up in the direction of the sunlight
        F32 distFromCentroid = std::max((maxZ - minZ), splitFrustumCornersVS[4].distance(splitFrustumCornersVS[5])) + nearClipOffset;
    
        const vec3<F32>& lightPosition = light.getPosition();
        vec3<F32> currentEye = frustumCentroid - (lightPosition * distFromCentroid);

        const mat4<F32>& viewMatrix = light.shadowCameras()[pass]->lookAt(currentEye, Normalize(frustumCentroid - currentEye));
        // Determine the position of the frustum corners in light space
        for (U8 i = 0; i < 8; ++i) {
            frustumCornersLS[i].set(viewMatrix.transformHomogeneous(frustumWS[i]));
        }

        mat4<F32> shadowMatrix = light.getShadowVPMatrix(pass);

        F32 frustumSphereRadius = BoundingSphere(frustumCornersLS).getRadius();
        vec2<F32> clipPlanes(std::max(1.0f, minZ - nearClipOffset), frustumSphereRadius * 2 + nearClipOffset * 2);
        const mat4<F32>& projMatrix = light.shadowCameras()[pass]->setProjection(UNIT_RECT * frustumSphereRadius, clipPlanes);

        mat4<F32>::Multiply(viewMatrix, projMatrix, shadowMatrix);

        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = g_shadowSettings.shadowMapResolution * 0.5f;
        vec3<F32> testPoint = shadowMatrix.transformHomogeneous(VECTOR3_ZERO) * halfShadowMapSize;
        vec3<F32> testPointRounded(testPoint);
        testPointRounded.round();
        shadowMatrix.translate(vec3<F32>(((testPointRounded - testPoint) / halfShadowMapSize).xy(), 0.0f));

        light.setShadowVPMatrix(pass, shadowMatrix);
        light.setShadowLightPos(pass, currentEye);
    }
}

void CascadedShadowMapsGenerator::postRender(const DirectionalLight& light, GFX::CommandBuffer& bufferInOut) {
    RenderTargetID depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::LAYERED));

    if (g_shadowSettings.enableBlurring) {
        PipelineDescriptor pipelineDescriptor;
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].push_back(_horizBlur);

        GenericDrawCommand pointsCmd;
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = 1;
        
        GFX::BeginRenderPassCommand beginRenderPassCmd;
        GFX::BindPipelineCommand pipelineCmd;
        GFX::SendPushConstantsCommand pushConstantsCommand;
        GFX::BindDescriptorSetsCommand descriptorSetCmd;
        GFX::EndRenderPassCommand endRenderPassCmd;
        GFX::DrawCommand drawCmd;
        drawCmd._drawCommands.push_back(pointsCmd);

        // Blur horizontally
        beginRenderPassCmd._target = _blurBuffer._targetID;
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_HORIZONTAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pipelineDescriptor._shaderProgramHandle = _blurDepthMapShader->getID();
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        _blurDepthMapConstants.set("layerCount", GFX::PushConstantType::INT, (I32)light.csmSplitCount());

        pushConstantsCommand._constants = _blurDepthMapConstants;
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        TextureData texData = _drawBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

        // Blur vertically
        beginRenderPassCmd._target = depthMapID;
        beginRenderPassCmd._descriptor = RenderTarget::defaultPolicyNoClear();
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_VERTICAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].front() = _vertBlur;
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        pushConstantsCommand._constants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)light.getShadowOffset());
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        texData = _blurBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set = _context.newDescriptorSet();
        descriptorSetCmd._set->_textureData.addTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
    } else {
        GFX::BlitRenderTargetCommand blitRenderTargetCommand;
        blitRenderTargetCommand._source = _drawBuffer._targetID;
        blitRenderTargetCommand._destination = depthMapID;

        ColourBlitEntry blitEntry = {};
        for (U8 i = 0; i < light.csmSplitCount(); ++i) {
            blitEntry._inputLayer = i;
            blitEntry._outputLayer = light.getShadowOffset() + i;
            blitRenderTargetCommand._blitColours.emplace_back(blitEntry);
        }
        GFX::EnqueueCommand(bufferInOut, blitRenderTargetCommand);
    }
}

};
#include "stdafx.h"

#include "Headers/CascadedShadowMapsGenerator.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"

#include "ECS/Components/Headers/DirectionalLightComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"

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

    g_shadowSettings = context.context().config().rendering.shadowMapping;

    ShaderModuleDescriptor vertModule = {};
    vertModule._moduleType = ShaderType::VERTEX;
    vertModule._sourceFile = "baseVertexShaders.glsl";
    vertModule._variant = "FullScreenQuad";

    ShaderModuleDescriptor geomModule = {};
    geomModule._moduleType = ShaderType::GEOMETRY;
    geomModule._sourceFile = "blur.glsl";
    geomModule._variant = "GaussBlur";
    geomModule._defines.push_back(std::make_pair(Util::StringFormat("GS_MAX_INVOCATIONS %d", Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT), true));

    ShaderModuleDescriptor fragModule = {};
    fragModule._moduleType = ShaderType::FRAGMENT;
    fragModule._sourceFile = "blur.glsl";
    fragModule._variant = "GaussBlur";

    ShaderProgramDescriptor shaderDescriptor = {};
    shaderDescriptor._modules.push_back(vertModule);
    shaderDescriptor._modules.push_back(geomModule);
    shaderDescriptor._modules.push_back(fragModule);

    ResourceDescriptor blurDepthMapShader(Util::StringFormat("GaussBlur_%d_invocations", Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT));
    blurDepthMapShader.setThreadedLoading(false);
    blurDepthMapShader.setPropertyDescriptor(shaderDescriptor);

    _blurDepthMapShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), blurDepthMapShader);
    _blurDepthMapConstants.set("layerCount", GFX::PushConstantType::INT, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
    _blurDepthMapConstants.set("layerOffsetRead", GFX::PushConstantType::INT, (I32)0);
    _blurDepthMapConstants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)0);
    _horizBlur = 0;
    _vertBlur = 0;

    _horizBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsH");
    _vertBlur = _blurDepthMapShader->GetSubroutineIndex(ShaderType::GEOMETRY, "computeCoordsV");

    std::array<vec2<F32>, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT> blurSizes;
    blurSizes.fill(vec2<F32>(1.0f / g_shadowSettings.shadowMapResolution) /** g_shadowSettings.softness*/);

    for (U8 i = 1; i < Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT; ++i) {
        blurSizes[i] = blurSizes[i - 1] / 2;
    }

    _blurDepthMapConstants.set("blurSizes", GFX::PushConstantType::VEC2, blurSizes);

    SamplerDescriptor sampler = {};
    sampler._wrapU = TextureWrap::CLAMP_TO_EDGE;
    sampler._wrapV = TextureWrap::CLAMP_TO_EDGE;
    sampler._wrapW = TextureWrap::CLAMP_TO_EDGE;
    sampler._minFilter = TextureFilter::LINEAR;
    sampler._magFilter = TextureFilter::LINEAR;
    sampler._anisotropyLevel = 0;

    RenderTargetID depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::LAYERED));
    const RenderTarget& rt = _context.renderTargetPool().renderTarget(depthMapID);
    const TextureDescriptor& texDescriptor = rt.getAttachment(RTAttachmentType::Colour, 0).texture()->getDescriptor();
    // Draw FBO
    {
        // MSAA rendering is supported
        TextureType texType = g_shadowSettings.msaaSamples > 0 ? TextureType::TEXTURE_2D_ARRAY_MS : TextureType::TEXTURE_2D_ARRAY;

        TextureDescriptor depthMapDescriptor(texType, texDescriptor.baseFormat(), texDescriptor.dataType());
        depthMapDescriptor.setLayerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
        depthMapDescriptor.setSampler(sampler);
        depthMapDescriptor.msaaSamples(g_shadowSettings.msaaSamples);

        TextureDescriptor depthDescriptor(texType, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);
        depthDescriptor.setLayerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
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
        TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY, texDescriptor.baseFormat(), texDescriptor.dataType());
        blurMapDescriptor.setLayerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
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
}

CascadedShadowMapsGenerator::~CascadedShadowMapsGenerator()
{
    _context.renderTargetPool().deallocateRT(_blurBuffer);
    _context.renderTargetPool().deallocateRT(_drawBuffer);
}

void CascadedShadowMapsGenerator::render(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    DirectionalLightComponent& dirLight = static_cast<DirectionalLightComponent&>(light);

    U8 numSplits = dirLight.csmSplitCount();

    const vec2<F32>& nearFarPlanes = playerCamera.getZPlanes();
    const mat4<F32>& projectionMatrix = playerCamera.getProjectionMatrix();
    SplitDepths splitDepths = calculateSplitDepths(projectionMatrix, dirLight, nearFarPlanes);
    applyFrustumSplits(dirLight, playerCamera.getViewMatrix(), projectionMatrix, nearFarPlanes, numSplits, splitDepths);
    
    RenderPassManager::PassParams params = {};
    params._sourceNode = &light.getSGN();
    params._stage = RenderStage::SHADOW;
    params._target = _drawBuffer._targetID;
    params._bindTargets = false;
    params._pass = RenderPassType::COUNT;
    params._passVariant = to_U8(light.getLightType());
    params._minLoD = -1;

    GFX::BeginRenderPassCommand beginRenderPassCmd = {};
    beginRenderPassCmd._target = params._target;
    beginRenderPassCmd._name = "DO_CSM_PASS";
    beginRenderPassCmd._descriptor = {};
    GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

    GFX::EndRenderSubPassCommand endRenderSubPassCommand = {};

    GFX::BeginDebugScopeCommand beginDebugScopeCommand = {};
    GFX::EndDebugScopeCommand endDebugScopeCommand = {};

    RenderTarget::DrawLayerParams drawParams = {};
    drawParams._type = RTAttachmentType::Colour;
    drawParams._index = 0;

    RenderPassManager& rpm = _context.parent().renderPassManager();

    bool renderLastSplit = true;//_context.getFrameCount() % 2 == 0;
    I16 i = to_I16(numSplits) - (renderLastSplit ? 1 : 2);
    for (i; i >= 0; i--) {
        drawParams._layer = i;

        beginDebugScopeCommand._scopeName = Util::StringFormat("CSM_PASS_%d", i).c_str();
        GFX::EnqueueCommand(bufferInOut, beginDebugScopeCommand);

        GFX::BeginRenderSubPassCommand beginRenderSubPassCmd = {};
        beginRenderSubPassCmd._writeLayers.push_back(drawParams);
        GFX::EnqueueCommand(bufferInOut, beginRenderSubPassCmd);

        constexpr U32 stride = std::max(to_U32(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS), 6u);
        params._passIndex = (lightIndex * stride) + i;
        params._camera = light.shadowCameras()[i];
        //params._minLoD = i > 1 ? 1 : -1;
        params._minExtents.set(i > 1 ? 3.5f : (i > 0 ? 2.5f : 0.5f));

        rpm.doCustomPass(params, bufferInOut);

        GFX::EnqueueCommand(bufferInOut, endRenderSubPassCommand);
        GFX::EnqueueCommand(bufferInOut, endDebugScopeCommand);
    }

    GFX::EndRenderPassCommand endRenderPassCmd = {};
    endRenderPassCmd._autoResolveMSAAColour = g_shadowSettings.enableBlurring;
    GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

    postRender(dirLight, bufferInOut);
}

//Between 0 and 1, change these to check the results
F32 minDistance = 0.07f;
F32 maxDistance = 1.0f;
CascadedShadowMapsGenerator::SplitDepths CascadedShadowMapsGenerator::calculateSplitDepths(const mat4<F32>& projMatrix, DirectionalLightComponent& light, const vec2<F32>& nearFarPlanes) {
    SplitDepths depths = {};

    U8 numSplits = light.csmSplitCount();
    F32 nearClip = nearFarPlanes.min;
    F32 farClip = nearFarPlanes.max;
    F32 clipRange = farClip - nearClip;

    F32 minZ = nearClip + minDistance * clipRange;
    F32 maxZ = nearClip + maxDistance * clipRange;

    F32 range = maxZ - minZ;
    F32 ratio = maxZ / minZ;

    U8 i = 0;
    for (; i < numSplits; ++i) {
        F32 p = (i + 1) / to_F32(numSplits);
        F32 log = minZ * std::pow(ratio, p);
        F32 uniform = minZ + range * p;
        F32 d = g_shadowSettings.splitLambda * (log - uniform) + uniform;
        depths[i] = (d - nearClip) / clipRange;
        light.setShadowFloatValue(i, (nearClip + depths[i] * clipRange) * -1.0f);
    }

    for (; i < Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT; ++i) {
        depths[i] = std::numeric_limits<F32>::max();
        light.setShadowFloatValue(i, depths[i] * -1.0f);
    }

    return depths;
}

void CascadedShadowMapsGenerator::applyFrustumSplits(DirectionalLightComponent& light,
                                                     const mat4<F32>& viewMatrix,
                                                     const mat4<F32>& projectionMatrix,
                                                     const vec2<F32>& nearFarPlanes,
                                                     U8 numSplits,
                                                     const SplitDepths& splitDepths)
{
    vec3<F32> lightInitialDirection = Normalized(light.getDirection());
    const mat4<F32> invViewProj = GetInverse(mat4<F32>::Multiply(viewMatrix, projectionMatrix));

    for (U8 cascadeIterator = 0 ; cascadeIterator < numSplits; ++cascadeIterator) {
        vec3<F32> frustumCornersWS[8] =
        {
            {-1.0f,  1.0f, -1.0f},
            { 1.0f,  1.0f, -1.0f},
            { 1.0f, -1.0f, -1.0f},
            {-1.0f, -1.0f, -1.0f},
            {-1.0f,  1.0f,  1.0f},
            { 1.0f,  1.0f,  1.0f},
            { 1.0f, -1.0f,  1.0f},
            {-1.0f, -1.0f,  1.0f},
        };

        for (U8 i = 0; i < 8; ++i) {
            vec4<F32> inversePoint = invViewProj * vec4<F32>(frustumCornersWS[i], 1.0f);
            frustumCornersWS[i].set(inversePoint.xyz() / inversePoint.w);
        }

        F32 prevSplitDistance = cascadeIterator == 0 ? minDistance : splitDepths[cascadeIterator - 1];
        F32 splitDistance = splitDepths[cascadeIterator];

        for (U8 i = 0; i < 4; ++i) {
            vec3<F32> cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
            vec3<F32> nearCornerRay = cornerRay * prevSplitDistance;
            vec3<F32> farCornerRay = cornerRay * splitDistance;
            frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
            frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
        }
        
        vec3<F32> frustumCenter(0.0f);
        for (U8 i = 0; i < 8; ++i) {
            frustumCenter += frustumCornersWS[i];
        }

        frustumCenter /= 8.0f;

        F32 radius = 0.0f;
        for (U8 i = 0; i < 8; ++i) {
            F32 distance = (frustumCornersWS[i] - frustumCenter).length();
            radius = std::max(radius, distance);
        }
        radius = std::ceil(radius * 16.0f) / 16.0f;

        //ToDo: This might not be needed anymore
        radius *= 1.01f;

        vec3<F32> maxExtents(radius);
        vec3<F32> minExtents = -maxExtents;

        //Position the viewmatrix looking down the center of the frustum with an arbitrary lighht direction
        vec3<F32> lightDirection = frustumCenter - lightInitialDirection * -minExtents.z;

        Camera* lightCam = light.shadowCameras()[cascadeIterator];
        vec3<F32> cascadeExtents = maxExtents - minExtents;

        Rect<F32> orthoRect{};
        orthoRect.left = minExtents.x;
        orthoRect.right = maxExtents.x;
        orthoRect.bottom = minExtents.y;
        orthoRect.top = maxExtents.y;

        vec2<F32> clipPlanes(0.0001f, cascadeExtents.z);

        // Lets store the ortho rect in case we need it;
        mat4<F32> lightOrthoMatrix = lightCam->setProjection(orthoRect, clipPlanes);
        const mat4<F32>& lightViewMatrix = lightCam->lookAt(lightDirection, frustumCenter, WORLD_Y_AXIS);

        mat4<F32> shadowMatrix = mat4<F32>::Multiply(lightViewMatrix, lightOrthoMatrix);

        // The rounding matrix that ensures that shadow edges do not shimmer
        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        F32 halfShadowMapSize = g_shadowSettings.shadowMapResolution * 0.5f;
        vec4<F32> shadowOrigin(0.0f, 0.0f, 0.0f, 1.0f);
        shadowOrigin = shadowMatrix * shadowOrigin;
        shadowOrigin = shadowOrigin * halfShadowMapSize;

        vec4<F32> roundedOrigin = shadowOrigin;
        roundedOrigin.round();

        vec4<F32> roundOffset = roundedOrigin - shadowOrigin;
        roundOffset = roundOffset * 2.0f / g_shadowSettings.shadowMapResolution;
        roundOffset.z = 0.0f;
        roundOffset.w = 0.0f;

        lightOrthoMatrix.translate(roundOffset.xyz());
        
        // Use our adjusted matrix for actual rendering
        lightCam->setProjection(lightOrthoMatrix, clipPlanes, true);

        mat4<F32>& lightVP = light.getShadowVPMatrix(cascadeIterator);
        mat4<F32>::Multiply(lightViewMatrix, lightOrthoMatrix, lightVP);

        light.setShadowLightPos(cascadeIterator, lightDirection);
        light.setShadowVPMatrix(cascadeIterator, mat4<F32>::Multiply(lightVP, MAT4_BIAS));
    }
}

void CascadedShadowMapsGenerator::postRender(const DirectionalLightComponent& light, GFX::CommandBuffer& bufferInOut) {
    RenderTargetID depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::LAYERED));

    if (g_shadowSettings.enableBlurring) {
        PipelineDescriptor pipelineDescriptor = {};
        pipelineDescriptor._stateHash = _context.get2DStateBlock();
        pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].push_back(_horizBlur);

        GenericDrawCommand pointsCmd = {};
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = 1;
        
        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        GFX::BindPipelineCommand pipelineCmd = {};
        GFX::SendPushConstantsCommand pushConstantsCommand = {};
        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        GFX::EndRenderPassCommand endRenderPassCmd = {};
        GFX::DrawCommand drawCmd = { pointsCmd };

        // Blur horizontally
        TextureData texData = _drawBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set._textureData.setTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        beginRenderPassCmd._target = _blurBuffer._targetID;
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_HORIZONTAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pipelineDescriptor._shaderProgramHandle = _blurDepthMapShader->getGUID();
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);
        _blurDepthMapConstants.set("layerCount", GFX::PushConstantType::INT, (I32)light.csmSplitCount());

        pushConstantsCommand._constants = _blurDepthMapConstants;
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);

        // Blur vertically
        texData = _blurBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->getData();
        descriptorSetCmd._set._textureData.setTexture(texData, to_U8(ShaderProgram::TextureUsage::UNIT0));
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        beginRenderPassCmd._target = depthMapID;
        beginRenderPassCmd._descriptor = {};
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_VERTICAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pipelineDescriptor._shaderFunctions[to_base(ShaderType::GEOMETRY)].front() = _vertBlur;
        pipelineCmd._pipeline = _context.newPipeline(pipelineDescriptor);
        GFX::EnqueueCommand(bufferInOut, pipelineCmd);

        pushConstantsCommand._constants.set("layerOffsetWrite", GFX::PushConstantType::INT, (I32)light.getShadowOffset());
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, endRenderPassCmd);
    } else {
        GFX::BlitRenderTargetCommand blitRenderTargetCommand = {};
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

    const RenderTarget& rt = _context.renderTargetPool().renderTarget(depthMapID);

    GFX::ComputeMipMapsCommand computeMipMapsCommand = {};
    computeMipMapsCommand._texture = rt.getAttachment(RTAttachmentType::Colour, 0).texture().get();
    computeMipMapsCommand._layerRange = vec2<U32>(light.getShadowOffset(), light.csmSplitCount());
    GFX::EnqueueCommand(bufferInOut, computeMipMapsCommand);
}

};
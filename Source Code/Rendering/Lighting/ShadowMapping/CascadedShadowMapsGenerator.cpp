#include "stdafx.h"

#include "Headers/CascadedShadowMapsGenerator.h"

#include "Rendering/Headers/Renderer.h"
#include "Rendering/Camera/Headers/Camera.h"

#include "ECS/Components/Headers/DirectionalLightComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/BoundsComponent.h"

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
    const RenderTargetID g_depthMapID(RenderTargetUsage::SHADOW, to_base(ShadowType::LAYERED));
    Configuration::Rendering::ShadowMapping g_shadowSettings;
};

CascadedShadowMapsGenerator::CascadedShadowMapsGenerator(GFXDevice& context)
    : ShadowMapGenerator(context, ShadowType::LAYERED)
{
    Console::printfn(Locale::get(_ID("LIGHT_CREATE_SHADOW_FB")), "EVCSM");
    const RenderTarget& rt = _context.renderTargetPool().renderTarget(g_depthMapID);

    g_shadowSettings = context.context().config().rendering.shadowMapping;
    {
        ShaderModuleDescriptor vertModule = {};
        vertModule._moduleType = ShaderType::VERTEX;
        vertModule._sourceFile = "baseVertexShaders.glsl";
        vertModule._variant = "FullScreenQuad";

        ShaderModuleDescriptor geomModule = {};
        geomModule._moduleType = ShaderType::GEOMETRY;
        geomModule._sourceFile = "blur.glsl";
        geomModule._variant = "GaussBlur";
        geomModule._defines.emplace_back(Util::StringFormat("GS_MAX_INVOCATIONS %d", Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT).c_str(), true);

        ShaderModuleDescriptor fragModule = {};
        fragModule._moduleType = ShaderType::FRAGMENT;
        fragModule._sourceFile = "blur.glsl";
        fragModule._variant = "GaussBlur";

        ShaderProgramDescriptor shaderDescriptor = {};
        shaderDescriptor._modules.push_back(vertModule);
        shaderDescriptor._modules.push_back(geomModule);
        shaderDescriptor._modules.push_back(fragModule);

        ResourceDescriptor blurDepthMapShader(Util::StringFormat("GaussBlur_%d_invocations", Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT).c_str());
        blurDepthMapShader.waitForReady(true);
        blurDepthMapShader.propertyDescriptor(shaderDescriptor);

        _blurDepthMapShader = CreateResource<ShaderProgram>(context.parent().resourceCache(), blurDepthMapShader);
        _blurDepthMapShader->addStateCallback(ResourceState::RES_LOADED, [this](CachedResource* res) {
            PipelineDescriptor pipelineDescriptor = {};
            pipelineDescriptor._stateHash = _context.get2DStateBlock();
            pipelineDescriptor._shaderProgramHandle = _blurDepthMapShader->getGUID();
            _blurPipeline = _context.newPipeline(pipelineDescriptor);
        });
    }

    PipelineDescriptor pipelineDescriptor = {};
    pipelineDescriptor._stateHash = _context.get2DStateBlock();

    _shaderConstants.set(_ID("layerCount"), GFX::PushConstantType::INT, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
    _shaderConstants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, (I32)0);
    _shaderConstants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, (I32)0);

    std::array<vec2<F32>, Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT> blurSizes;
    blurSizes.fill(vec2<F32>(1.0f / g_shadowSettings.csm.shadowMapResolution) /** g_shadowSettings.softness*/);

    for (U8 i = 1; i < Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT; ++i) {
        blurSizes[i] = blurSizes[i - 1] / 2;
    }

    _shaderConstants.set(_ID("blurSizes"), GFX::PushConstantType::VEC2, blurSizes);

    SamplerDescriptor sampler = {};
    sampler.wrapU(TextureWrap::CLAMP_TO_EDGE);
    sampler.wrapV(TextureWrap::CLAMP_TO_EDGE);
    sampler.wrapW(TextureWrap::CLAMP_TO_EDGE);
    sampler.minFilter(TextureFilter::LINEAR);
    sampler.magFilter(TextureFilter::LINEAR);

    sampler.anisotropyLevel(0);

    const TextureDescriptor& texDescriptor = rt.getAttachment(RTAttachmentType::Colour, 0).texture()->descriptor();
    // Draw FBO
    {
        // MSAA rendering is supported
        TextureDescriptor colourDescriptor(TextureType::TEXTURE_2D_ARRAY_MS, texDescriptor.baseFormat(), texDescriptor.dataType());
        colourDescriptor.layerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
        colourDescriptor.samplerDescriptor(sampler);
        colourDescriptor.msaaSamples(g_shadowSettings.csm.MSAASamples);

        TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D_ARRAY_MS, GFXImageFormat::DEPTH_COMPONENT, GFXDataFormat::UNSIGNED_INT);
        depthDescriptor.layerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
        depthDescriptor.samplerDescriptor(sampler);
        depthDescriptor.msaaSamples(g_shadowSettings.csm.MSAASamples);

        vectorEASTL<RTAttachmentDescriptor> att = {
            { colourDescriptor, RTAttachmentType::Colour },
            { depthDescriptor, RTAttachmentType::Depth }
        };

        RenderTargetDescriptor desc = {};
        desc._resolution = rt.getResolution();
        desc._name = "CSM_ShadowMap_Draw";
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();
        desc._msaaSamples = g_shadowSettings.csm.MSAASamples;

        _drawBufferDepth = context.renderTargetPool().allocateRT(desc);
    }

    //Blur FBO
    {
        TextureDescriptor blurMapDescriptor(TextureType::TEXTURE_2D_ARRAY, texDescriptor.baseFormat(), texDescriptor.dataType());
        blurMapDescriptor.layerCount(Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT);
        blurMapDescriptor.samplerDescriptor(sampler);

        vectorEASTL<RTAttachmentDescriptor> att = {
            { blurMapDescriptor, RTAttachmentType::Colour }
        };

        RenderTargetDescriptor desc = {};
        desc._name = "CSM_Blur";
        desc._resolution = rt.getResolution();
        desc._attachmentCount = to_U8(att.size());
        desc._attachments = att.data();

        _blurBuffer = _context.renderTargetPool().allocateRT(desc);
    }

    WAIT_FOR_CONDITION(_blurPipeline != nullptr);
}

CascadedShadowMapsGenerator::~CascadedShadowMapsGenerator()
{
    _context.renderTargetPool().deallocateRT(_blurBuffer);
    _context.renderTargetPool().deallocateRT(_drawBufferDepth);
}

bool CascadedShadowMapsGenerator::useMSAA() const noexcept {
    return g_shadowSettings.csm.MSAASamples > 0u;
}

//Between 0 and 1, change these to check the results
constexpr F32 minDistance = 0.0f;
constexpr F32 maxDistance = 1.0f;
CascadedShadowMapsGenerator::SplitDepths CascadedShadowMapsGenerator::calculateSplitDepths(const mat4<F32>& projMatrix, DirectionalLightComponent& light, const vec2<F32>& nearFarPlanes) noexcept {
    SplitDepths depths = {};

    const U8 numSplits = light.csmSplitCount();
    const F32 nearClip = nearFarPlanes.min;
    const F32 farClip = nearFarPlanes.max;
    const F32 clipRange = farClip - nearClip;

    const F32 minZ = nearClip + minDistance * clipRange;
    const F32 maxZ = nearClip + maxDistance * clipRange;

    const F32 range = maxZ - minZ;
    const F32 ratio = maxZ / minZ;

    U8 i = 0;
    for (; i < numSplits; ++i) {
        const F32 p = (i + 1) / to_F32(numSplits);
        const F32 log = minZ * std::pow(ratio, p);
        const F32 uniform = minZ + range * p;
        const F32 d = g_shadowSettings.csm.splitLambda * (log - uniform) + uniform;
        depths[i] = (d - nearClip) / clipRange;
        light.setShadowFloatValue(i, d);
    }

    for (; i < Config::Lighting::MAX_CSM_SPLITS_PER_LIGHT; ++i) {
        depths[i] = std::numeric_limits<F32>::max();
        light.setShadowFloatValue(i, -depths[i]);
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
    const mat4<F32> invViewProj = GetInverse(mat4<F32>::Multiply(viewMatrix, projectionMatrix));

    F32 appliedDiff = 0.0f;
    for (U8 cascadeIterator = 0; cascadeIterator < numSplits; ++cascadeIterator) {
        Camera* lightCam = ShadowMap::shadowCameras(ShadowType::LAYERED)[cascadeIterator];

        const F32 prevSplitDistance = cascadeIterator == 0 ? 0.0f : splitDepths[cascadeIterator - 1];
        const F32 splitDistance = splitDepths[cascadeIterator];

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
            const vec4<F32> inversePoint = invViewProj * vec4<F32>(frustumCornersWS[i], 1.0f);
            frustumCornersWS[i].set(inversePoint / inversePoint.w);
        }

        for (U8 i = 0; i < 4; ++i) {
            const vec3<F32> cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
            const vec3<F32> nearCornerRay = cornerRay * prevSplitDistance;
            const vec3<F32> farCornerRay = cornerRay * splitDistance;

            frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
            frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
        }

        vec3<F32> frustumCenter = VECTOR3_ZERO;
        for (U8 i = 0; i < 8; ++i) {
            frustumCenter += frustumCornersWS[i];
        }
        frustumCenter /= 8.0f;

        F32 radius = 0.0f;
        for (U8 i = 0; i < 8; ++i) {
            const F32 distance = (frustumCornersWS[i] - frustumCenter).lengthSquared();
            radius = std::max(radius, distance);
        }
        radius = std::ceil(Sqrt(radius) * 16.0f) / 16.0f;
        radius += appliedDiff;

        vec3<F32> maxExtents(radius, radius, radius);
        vec3<F32> minExtents = -maxExtents;

        //Position the viewmatrix looking down the center of the frustum with an arbitrary light direction
        vec3<F32> lightPosition = frustumCenter - light.directionCache() * (light.csmNearClipOffset() - minExtents.z);
        mat4<F32> lightViewMatrix = lightCam->lookAt(lightPosition, frustumCenter, WORLD_Y_AXIS);

        if (light.csmUseSceneAABBFit()) {
            // Only meshes should be enough
            bool validResult = false;
            auto& prevPassResults = light.feedBackContainers()[cascadeIterator]._visibleNodes;
            if (!prevPassResults.empty()) {
                BoundingBox meshAABB = {};
                for (auto& node : prevPassResults) {
                    const SceneNode& sNode = node._node->getNode();
                    if (sNode.type() == SceneNodeType::TYPE_OBJECT3D) {
                        if (static_cast<const Object3D&>(sNode).getObjectType()._value == ObjectType::SUBMESH) {
                            meshAABB.add(node._node->get<BoundsComponent>()->getBoundingBox());
                            validResult = true;
                        }
                    }
                }

                if (validResult) {
                    meshAABB.transform(lightViewMatrix);
                    const F32 newMax = meshAABB.getHalfExtent().y;

                    appliedDiff = newMax - radius;
                    if (appliedDiff > 0.5f) {
                        radius += appliedDiff * 0.75f;

                        maxExtents.set(radius, radius, radius);
                        minExtents = -maxExtents;

                        //Position the viewmatrix looking down the center of the frustum with an arbitrary light direction
                        lightPosition = frustumCenter - light.directionCache() * (light.csmNearClipOffset() - minExtents.z);
                        lightViewMatrix = lightCam->lookAt(lightPosition, frustumCenter, WORLD_Y_AXIS);
                    }
                }
            }
        }

        // Lets store the ortho rect in case we need it;
        const vec2<F32> clip = {
            0.001f,
            maxExtents.z - minExtents.z
        };

        mat4<F32> lightOrthoMatrix = {
            Rect<F32>{
                minExtents.x,
                maxExtents.x,
                minExtents.y,
                maxExtents.y
            },
            clip
        };

        // The rounding matrix that ensures that shadow edges do not shimmer
        // http://www.gamedev.net/topic/591684-xna-40---shimmering-shadow-maps/
        {
            const mat4<F32> shadowMatrix = mat4<F32>::Multiply(lightViewMatrix, lightOrthoMatrix);
            vec4<F32> shadowOrigin = {0.0f, 0.0f, 0.0f, 1.0f };
            shadowOrigin = shadowMatrix * shadowOrigin;
            shadowOrigin = shadowOrigin * g_shadowSettings.csm.shadowMapResolution * 0.5f;

            vec4<F32> roundedOrigin = shadowOrigin; 
            roundedOrigin.round();

            vec3<F32> roundOffset = (roundedOrigin - shadowOrigin).xyz() * 2.0f / g_shadowSettings.csm.shadowMapResolution;
            roundOffset.z = 0.0f;

            lightOrthoMatrix.translate(roundOffset);

            // Use our adjusted matrix for actual rendering
            lightCam->setProjection(lightOrthoMatrix, clip, true);
        }
        lightCam->updateLookAt();

        mat4<F32>& lightVP = light.getShadowVPMatrix(cascadeIterator);
        mat4<F32>::Multiply(lightViewMatrix, lightOrthoMatrix, lightVP);

        light.setShadowLightPos(cascadeIterator, lightPosition);
        light.setShadowVPMatrix(cascadeIterator, mat4<F32>::Multiply(lightVP, MAT4_BIAS));
    }
}

void CascadedShadowMapsGenerator::render(const Camera& playerCamera, Light& light, U16 lightIndex, GFX::CommandBuffer& bufferInOut) {

    DirectionalLightComponent& dirLight = static_cast<DirectionalLightComponent&>(light);

    const U8 numSplits = dirLight.csmSplitCount();

    const vec2<F32>& nearFarPlanes = playerCamera.getZPlanes();
    const mat4<F32>& projectionMatrix = playerCamera.getProjectionMatrix();
    const SplitDepths splitDepths = calculateSplitDepths(projectionMatrix, dirLight, nearFarPlanes);
    applyFrustumSplits(dirLight, playerCamera.getViewMatrix(), projectionMatrix, nearFarPlanes, numSplits, splitDepths);
    
    RenderPassManager::PassParams params = {};
    params._sourceNode = &light.getSGN();
    params._stagePass = RenderStagePass(RenderStage::SHADOW, RenderPassType::COUNT, to_U8(light.getLightType()), lightIndex);
    params._target = _drawBufferDepth._targetID;
    params._minLoD = -1;
    params._layerParams._type = RTAttachmentType::Colour;
    params._layerParams._index = 0;

    GFX::EnqueueCommand(bufferInOut, GFX::BeginDebugScopeCommand({ Util::StringFormat("Cascaded Shadow Pass Light: [ %d ]", lightIndex).c_str() }));

    RTClearDescriptor clearDescriptor = {}; 
    clearDescriptor.clearDepth(true);
    clearDescriptor.clearColours(true);
    clearDescriptor.resetToDefault(true);

    GFX::ClearRenderTargetCommand clearMainTarget = {};
    clearMainTarget._target = params._target;
    clearMainTarget._descriptor = clearDescriptor;
    GFX::EnqueueCommand(bufferInOut, clearMainTarget);

    RenderPassManager* rpm = _context.parent().renderPassManager();

    constexpr F32 minExtentsFactors[] = { 0.025f, 2.0f, 10.5f, 50.5f };

    I16 i = to_I16(numSplits) - 1;
    for (i; i >= 0; i--) {
        params._layerParams._layer = i;
        params._passName = Util::StringFormat("CSM_PASS_%d", i).c_str();
        params._stagePass._pass = i;
        params._camera = ShadowMap::shadowCameras(ShadowType::LAYERED)[i];
        params._minExtents.set(minExtentsFactors[i]);
        if (dirLight.csmUseSceneAABBFit()) {
            params._feedBackContainer = &dirLight.feedBackContainers()[i];
            params._feedBackContainer->_visibleNodes.resize(0);
        }

        rpm->doCustomPass(params, bufferInOut);
    }

    postRender(dirLight, bufferInOut);

    GFX::EnqueueCommand(bufferInOut, GFX::EndDebugScopeCommand{});
}

void CascadedShadowMapsGenerator::postRender(const DirectionalLightComponent& light, GFX::CommandBuffer& bufferInOut) {
    const RenderTarget& shadowMapRT = _context.renderTargetPool().renderTarget(g_depthMapID);

    const I32 layerOffset = to_I32(light.getShadowOffset());
    const I32 layerCount = to_I32(light.csmSplitCount());

    GFX::BlitRenderTargetCommand blitRenderTargetCommand = {};
    blitRenderTargetCommand._source = _drawBufferDepth._targetID;
    blitRenderTargetCommand._destination = g_depthMapID;

    for (U8 i = 0; i < light.csmSplitCount(); ++i) {
        blitRenderTargetCommand._blitColours[i].set(0u, 0u, i, to_U16(layerOffset + i));
    }

    GFX::EnqueueCommand(bufferInOut, blitRenderTargetCommand);

    // Now we can either blur our target or just skip to mipmap computation
    if (g_shadowSettings.csm.enableBlurring) {
        _shaderConstants.set(_ID("layerCount"), GFX::PushConstantType::INT, layerCount);

        GenericDrawCommand pointsCmd = {};
        pointsCmd._primitiveType = PrimitiveType::API_POINTS;
        pointsCmd._drawCount = 1;

        GFX::DrawCommand drawCmd = { pointsCmd };
        GFX::BindDescriptorSetsCommand descriptorSetCmd = {};
        GFX::BeginRenderPassCommand beginRenderPassCmd = {};
        GFX::SendPushConstantsCommand pushConstantsCommand = {};

        // Blur horizontally
        beginRenderPassCmd._target = _blurBuffer._targetID;
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_HORIZONTAL";

        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::BindPipelineCommand{ _blurPipeline });

        TextureData texData = shadowMapRT.getAttachment(RTAttachmentType::Colour, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(texData, TextureUsage::UNIT0);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        _shaderConstants.set(_ID("layered"), GFX::PushConstantType::BOOL, true);
        _shaderConstants.set(_ID("verticalBlur"), GFX::PushConstantType::BOOL, false);
        _shaderConstants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, layerOffset);
        _shaderConstants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, 0);

        pushConstantsCommand._constants = _shaderConstants;
        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});

        // Blur vertically
        texData = _blurBuffer._rt->getAttachment(RTAttachmentType::Colour, 0).texture()->data();
        descriptorSetCmd._set._textureData.setTexture(texData, TextureUsage::UNIT0);
        GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);

        beginRenderPassCmd._target = g_depthMapID;
        beginRenderPassCmd._descriptor = {};
        beginRenderPassCmd._name = "DO_CSM_BLUR_PASS_VERTICAL";
        GFX::EnqueueCommand(bufferInOut, beginRenderPassCmd);

        pushConstantsCommand._constants.set(_ID("verticalBlur"), GFX::PushConstantType::BOOL, true);
        pushConstantsCommand._constants.set(_ID("layerOffsetRead"), GFX::PushConstantType::INT, 0);
        pushConstantsCommand._constants.set(_ID("layerOffsetWrite"), GFX::PushConstantType::INT, layerOffset);

        GFX::EnqueueCommand(bufferInOut, pushConstantsCommand);

        GFX::EnqueueCommand(bufferInOut, drawCmd);

        GFX::EnqueueCommand(bufferInOut, GFX::EndRenderPassCommand{});
    }

    GFX::ComputeMipMapsCommand computeMipMapsCommand = {};
    computeMipMapsCommand._texture = shadowMapRT.getAttachment(RTAttachmentType::Colour, 0).texture().get();
    computeMipMapsCommand._layerRange = vec2<U32>(light.getShadowOffset(), light.csmSplitCount());
    computeMipMapsCommand._defer = false;
    GFX::EnqueueCommand(bufferInOut, computeMipMapsCommand);
}

void CascadedShadowMapsGenerator::updateMSAASampleCount(U8 sampleCount) {
    if (_context.context().config().rendering.shadowMapping.csm.MSAASamples != sampleCount) {
        _context.context().config().rendering.shadowMapping.csm.MSAASamples = sampleCount;
        _drawBufferDepth._rt->updateSampleCount(sampleCount);
    }
}
};
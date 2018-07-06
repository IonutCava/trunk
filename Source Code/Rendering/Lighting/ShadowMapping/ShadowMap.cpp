#include "stdafx.h"

#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMapGenerator.h"
#include "Headers/SingleShadowMapGenerator.h"
#include "Headers/CascadedShadowMapsGenerator.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"

#include "Scenes/Headers/SceneState.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

std::array<ShadowMap::LayerUsageMask, to_base(ShadowType::COUNT)> ShadowMap::s_depthMapUsage;
std::array<ShadowMapGenerator*, to_base(ShadowType::COUNT)> ShadowMap::s_shadowMapGenerators;

vector<RenderTargetHandle> ShadowMap::s_shadowMaps;



ShadowMapGenerator::ShadowMapGenerator(GFXDevice& context)
    : _context(context)
{
}

ShadowType ShadowMap::getShadowTypeForLightType(LightType type) {
    switch (type) {
    case LightType::DIRECTIONAL: return ShadowType::LAYERED;
    case LightType::POINT: return ShadowType::CUBEMAP;
    case LightType::SPOT: return ShadowType::SINGLE;
    }

    return ShadowType::COUNT;
}

void ShadowMap::initShadowMaps(GFXDevice& context) {
    Configuration::Rendering::ShadowMapping& settings = context.parent().platformContext().config().rendering.shadowMapping;
    
    if (!isPowerOfTwo(settings.shadowMapResolution)) {
        settings.shadowMapResolution = nextPOW2(settings.shadowMapResolution);
    }
    
    std::array<U16, to_base(ShadowType::COUNT)> resolutions;
    resolutions.fill(settings.shadowMapResolution);
    resolutions[to_base(ShadowType::CUBEMAP)] = settings.shadowMapResolution / 2;

    RenderTargetHandle crtTarget;
    for (U32 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        switch (static_cast<ShadowType>(i)) {
            case ShadowType::SINGLE: {
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.setFilters(TextureFilter::LINEAR);
                depthMapSampler._useRefCompare = true;
                depthMapSampler._cmpFunc = ComparisonFunction::LEQUAL;

                // Default filters, LINEAR is OK for this
                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                                     GFXImageFormat::DEPTH_COMPONENT,
                                                     GFXDataFormat::UNSIGNED_INT);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthMapDescriptor.setSampler(depthMapSampler);

                vector<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Depth },
                };

                RenderTargetDescriptor desc = {};
                desc._name = "Single_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.renderTargetPool().allocateRT(RenderTargetUsage::SHADOW, desc);

                s_shadowMapGenerators[i] = MemoryManager_NEW SingleShadowMapGenerator(context);
            } break;

            case ShadowType::LAYERED: {
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setFilters(TextureFilter::LINEAR_MIPMAP_LINEAR);
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.setAnisotropy(settings.anisotropicFilteringLevel);

                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                                     GFXImageFormat::RG32F,
                                                     GFXDataFormat::FLOAT_32);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT * Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthMapDescriptor.setSampler(depthMapSampler);

                vector<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Colour }
                };

                RenderTargetDescriptor desc = {};
                desc._name = "CSM_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.renderTargetPool().allocateRT(RenderTargetUsage::SHADOW, desc);

                s_shadowMapGenerators[i] = MemoryManager_NEW CascadedShadowMapsGenerator(context);
            } break;

            case ShadowType::CUBEMAP: {
                // Default filters, LINEAR is OK for this
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.setFilters(TextureFilter::LINEAR);
                depthMapSampler._useRefCompare = true;  //< Use compare function
                depthMapSampler._cmpFunc = ComparisonFunction::LEQUAL;  //< Use less or equal

                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_CUBE_ARRAY,
                                                     GFXImageFormat::DEPTH_COMPONENT,
                                                     GFXDataFormat::UNSIGNED_INT);
                depthMapDescriptor.setSampler(depthMapSampler);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

                vector<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Depth },
                };

                RenderTargetDescriptor desc = {};
                desc._name = "Cube_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.renderTargetPool().allocateRT(RenderTargetUsage::SHADOW, desc);

                s_shadowMapGenerators[i] = MemoryManager_NEW CubeShadowMapGenerator(context);
            } break;
        };
        s_shadowMaps.push_back(crtTarget);

        s_depthMapUsage[i].resize(0);
    }
}

void ShadowMap::destroyShadowMaps(GFXDevice& context) {
    for (RenderTargetHandle& handle : s_shadowMaps) {
        context.renderTargetPool().deallocateRT(handle);
    }
    s_shadowMaps.clear();

    for (ShadowMapGenerator* smg : s_shadowMapGenerators) {
        MemoryManager::DELETE(smg);
    }

    s_shadowMapGenerators.fill(nullptr);
}

void ShadowMap::resetShadowMaps() {
    for (U32 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        s_depthMapUsage[i].resize(0);
    }
}

void ShadowMap::bindShadowMaps(GFXDevice& context, GFX::CommandBuffer& bufferInOut) {
    GFX::BindDescriptorSetsCommand descriptorSetCmd;
    descriptorSetCmd._set = context.newDescriptorSet();
    for (U8 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        RTAttachmentType attachment = static_cast<ShadowType>(i) == ShadowType::LAYERED
                                                                  ? RTAttachmentType::Colour
                                                                  : RTAttachmentType::Depth;

        U8 bindSlot = LightPool::getShadowBindSlotOffset(static_cast<ShadowType>(i));
        RTAttachment& shadowTexture = context.renderTargetPool().renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i)).getAttachment(attachment, 0);
        descriptorSetCmd._set->_textureData.addTexture(shadowTexture.texture()->getData(), bindSlot);
    }
    GFX::EnqueueCommand(bufferInOut, descriptorSetCmd);
}

void ShadowMap::clearShadowMapBuffers(GFX::CommandBuffer& bufferInOut) {
    GFX::ResetRenderTargetCommand resetRenderTargetCommand;
    for (U8 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        resetRenderTargetCommand._source = RenderTargetID(RenderTargetUsage::SHADOW, i);
        resetRenderTargetCommand._descriptor = RenderTarget::defaultPolicy();
        GFX::EnqueueCommand(bufferInOut, resetRenderTargetCommand);
    }
}

U16 ShadowMap::findDepthMapOffset(ShadowType shadowType) {
    LayerUsageMask& usageMask = s_depthMapUsage[to_U32(shadowType)];
    U16 layer = std::numeric_limits<U16>::max();
    U16 usageCount = (U16)usageMask.size();
    for (U16 i = 0; i < usageCount; ++i) {
        if (usageMask[i] == false) {
            layer = i;
            break;
        }
    }

    if (layer > usageCount) {
        layer = usageCount;
        usageMask.push_back(true);
    }

    return layer;
}

void ShadowMap::commitDepthMapOffset(ShadowType shadowType, U32 layer) {
    LayerUsageMask& usageMask = s_depthMapUsage[to_U32(shadowType)];
    usageMask[layer] = true;
}

bool ShadowMap::freeDepthMapOffset(ShadowType shadowType, U32 layer) {
    LayerUsageMask& usageMask = s_depthMapUsage[to_U32(shadowType)];
    usageMask[layer] = false;

    return true;
}

void ShadowMap::generateShadowMaps(const Camera& playerCamera, Light& light, U32 lightIndex, GFX::CommandBuffer& bufferInOut) {
    ShadowType sType = getShadowTypeForLightType(light.getLightType());
    U16 offset = findDepthMapOffset(sType);
    light.setShadowOffset(offset);
    commitDepthMapOffset(sType, offset);

    s_shadowMapGenerators[to_base(sType)]->render(playerCamera, light, lightIndex, bufferInOut);
}


const RenderTargetHandle& ShadowMap::getDepthMap(LightType type) {
    return s_shadowMaps[to_base(getShadowTypeForLightType(type))];
}

};
#include "stdafx.h"

#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMap.h"
#include "Headers/SingleShadowMap.h"
#include "Scenes/Headers/SceneState.h"
#include "Headers/CascadedShadowMaps.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

std::array<ShadowMap::LayerUsageMask, to_base(ShadowType::COUNT)> ShadowMap::_depthMapUsage;


ShadowMap::ShadowMap(GFXDevice& context, Light* light, const ShadowCameraPool& shadowCameras, ShadowType type)
    : _context(context),
      _init(false),
      _light(light),
      _shadowCameras(shadowCameras),
      _shadowMapType(type),
      _arrayOffset(findDepthMapLayer(_shadowMapType))
{
    commitDepthMapLayer(_shadowMapType, _arrayOffset);
}

ShadowMap::~ShadowMap()
{
    freeDepthMapLayer(_shadowMapType, _arrayOffset);
}

void ShadowMap::resetShadowMaps(GFXDevice& context) {
    ACKNOWLEDGE_UNUSED(context);

    for (U32 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        _depthMapUsage[i].fill(false);
    }
}

void ShadowMap::initShadowMaps(GFXDevice& context) {
    std::array<U16, to_base(ShadowType::COUNT)> resolutions;
    resolutions.fill(512);
    if (context.shadowDetailLevel() == RenderDetailLevel::ULTRA) {
        // Cap cubemap resolution
        resolutions.fill(2048);
        resolutions[to_base(ShadowType::CUBEMAP)] = 1024;
    } else if (context.shadowDetailLevel() == RenderDetailLevel::HIGH) {
        resolutions.fill(1024);
    }

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

                vectorImpl<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Depth },
                };

                RenderTargetDescriptor desc = {};
                desc._name = "Single_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, desc);
            } break;

            case ShadowType::LAYERED: {
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setFilters(TextureFilter::LINEAR_MIPMAP_LINEAR);
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.setAnisotropy(8);
                depthMapSampler._useRefCompare = false;

                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                                     GFXImageFormat::RG32F,
                                                     GFXDataFormat::FLOAT_32);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT *
                                                 Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthMapDescriptor.setSampler(depthMapSampler);

                SamplerDescriptor depthSampler;
                depthSampler.setFilters(TextureFilter::NEAREST);
                depthSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthSampler._useRefCompare = true;

                TextureDescriptor depthDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                                  GFXImageFormat::DEPTH_COMPONENT,
                                                  GFXDataFormat::UNSIGNED_INT);
                depthDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT *
                                              Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthDescriptor.setSampler(depthSampler);

                vectorImpl<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Colour },
                    { depthDescriptor, RTAttachmentType::Depth },
                };

                RenderTargetDescriptor desc = {};
                desc._name = "CSM_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, desc);

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

                vectorImpl<RTAttachmentDescriptor> att = {
                    { depthMapDescriptor, RTAttachmentType::Depth },
                };

                RenderTargetDescriptor desc = {};
                desc._name = "Cube_ShadowMap";
                desc._resolution = resolutions[i];
                desc._attachmentCount = to_U8(att.size());
                desc._attachments = att.data();

                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, desc);
            } break;
        };

        _depthMapUsage[i].fill(false);
    }
}

void ShadowMap::clearShadowMaps(GFXDevice& context) {
    for (U32 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i), nullptr);
    }
}

void ShadowMap::bindShadowMaps(GFXDevice& context) {
    for (U8 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        RTAttachmentType attachment
            = static_cast<ShadowType>(i) == ShadowType::LAYERED
                                          ? RTAttachmentType::Colour
                                          : RTAttachmentType::Depth;

        U8 bindSlot = LightPool::getShadowBindSlotOffset(static_cast<ShadowType>(i));
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i)).bind(bindSlot, attachment, 0);
    }
}

void ShadowMap::clearShadowMapBuffers(GFXDevice& context) {
    /*for (U8 i = 0; i < to_base(ShadowType::COUNT); ++i) {
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i)).clear(RenderTarget::defaultPolicy());
    }*/
}

U16 ShadowMap::findDepthMapLayer(ShadowType shadowType) {
    LayerUsageMask& usageMask = _depthMapUsage[to_U32(shadowType)];
    U16 layer = std::numeric_limits<U16>::max();
    for (U16 i = 0; i < to_base(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS); ++i) {
        if (usageMask[i] == false) {
            layer = i;
            break;
        }
    }

    return layer;
}

RenderTargetID ShadowMap::getDepthMapID() {
    return RenderTargetID(RenderTargetUsage::SHADOW, to_U32(getShadowMapType()));
}

const RenderTargetID ShadowMap::getDepthMapID() const {
    return RenderTargetID(RenderTargetUsage::SHADOW, to_U32(getShadowMapType()));
}

RenderTarget& ShadowMap::getDepthMap() {
    return _context.renderTarget(getDepthMapID());
}

const RenderTarget& ShadowMap::getDepthMap() const {
    return _context.renderTarget(getDepthMapID());
}

void ShadowMap::commitDepthMapLayer(ShadowType shadowType, U32 layer) {
    DIVIDE_ASSERT(layer < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS,
        "ShadowMap::commitDepthMapLayer error: Insufficient texture layers for shadow mapping");
    LayerUsageMask& usageMask = _depthMapUsage[to_U32(shadowType)];
    usageMask[layer] = true;
}

bool ShadowMap::freeDepthMapLayer(ShadowType shadowType, U32 layer) {
    DIVIDE_ASSERT(layer < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS,
        "ShadowMap::freeDepthMapLayer error: Invalid layer value");
    LayerUsageMask& usageMask = _depthMapUsage[to_U32(shadowType)];
    usageMask[layer] = false;

    return true;
}

ShadowMapInfo::ShadowMapInfo(Light* light)
    : _light(light),
      _shadowMap(nullptr)
{

    _numLayers = 1;
}

ShadowMapInfo::~ShadowMapInfo()
{
    MemoryManager::DELETE(_shadowMap);
}

ShadowMap* ShadowMapInfo::createShadowMap(GFXDevice& context, const SceneRenderState& renderState, const ShadowCameraPool& shadowCameras) {
    if (_shadowMap) {
        return _shadowMap;
    }

    if (!_light->castsShadows()) {
        return nullptr;
    }

    switch (_light->getLightType()) {
        case LightType::POINT: {
            _numLayers = 6;
            _shadowMap = MemoryManager_NEW CubeShadowMap(context, _light, shadowCameras);
        } break;
        case LightType::DIRECTIONAL: {
            DirectionalLight* dirLight = static_cast<DirectionalLight*>(_light);
            _numLayers = dirLight->csmSplitCount();
            _shadowMap = MemoryManager_NEW CascadedShadowMaps(context, _light, shadowCameras, _numLayers);
        } break;
        case LightType::SPOT: {
            _shadowMap = MemoryManager_NEW SingleShadowMap(context, _light, shadowCameras);
        } break;
        default:
            break;
    };

    _shadowMap->init(this);

    return _shadowMap;
}

};
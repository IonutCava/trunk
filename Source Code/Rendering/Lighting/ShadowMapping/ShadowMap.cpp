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

std::array<ShadowMap::LayerUsageMask, to_const_uint(ShadowType::COUNT)> ShadowMap::_depthMapUsage;


ShadowMap::ShadowMap(GFXDevice& context, Light* light, Camera* shadowCamera, ShadowType type)
    : _context(context),
      _init(false),
      _light(light),
      _shadowCamera(shadowCamera),
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

    for (U32 i = 0; i < to_const_uint(ShadowType::COUNT); ++i) {
        _depthMapUsage[i].fill(false);
    }
}

void ShadowMap::initShadowMaps(GFXDevice& context) {
    std::array<U16, to_const_uint(ShadowType::COUNT)> resolutions;
    resolutions.fill(512);
    if (context.shadowDetailLevel() == RenderDetailLevel::ULTRA) {
        // Cap cubemap resolution
        resolutions.fill(2048);
        resolutions[to_const_uint(ShadowType::CUBEMAP)] = 1024;
    } else if (context.shadowDetailLevel() == RenderDetailLevel::HIGH) {
        resolutions.fill(1024);
    }

    RenderTargetHandle crtTarget;
    for (U32 i = 0; i < to_const_uint(ShadowType::COUNT); ++i) {
        switch (static_cast<ShadowType>(i)) {
            case ShadowType::SINGLE: {
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.toggleMipMaps(false);
                depthMapSampler.setFilters(TextureFilter::LINEAR);
                depthMapSampler._useRefCompare = true;
                depthMapSampler._cmpFunc = ComparisonFunction::LEQUAL;
                // Default filters, LINEAR is OK for this
                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_2D_ARRAY,
                                                     GFXImageFormat::DEPTH_COMPONENT,
                                                     GFXDataFormat::UNSIGNED_INT);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthMapDescriptor.setSampler(depthMapSampler);

                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, "Single_ShadowMap");
                crtTarget._rt->addAttachment(depthMapDescriptor, RTAttachment::Type::Depth, 0);
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
                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, "CSM_ShadowMap");
                depthDescriptor.setLayerCount(Config::Lighting::MAX_SPLITS_PER_LIGHT *
                                              Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);
                depthDescriptor.setSampler(depthSampler);
                crtTarget._rt->addAttachment(depthMapDescriptor, RTAttachment::Type::Colour, 0);
                crtTarget._rt->addAttachment(depthDescriptor, RTAttachment::Type::Depth, 0);
                crtTarget._rt->setClearColour(RTAttachment::Type::COUNT, 0, DefaultColours::WHITE());
            } break;

            case ShadowType::CUBEMAP: {
                // Default filters, LINEAR is OK for this
                SamplerDescriptor depthMapSampler;
                depthMapSampler.setWrapMode(TextureWrap::CLAMP_TO_EDGE);
                depthMapSampler.setFilters(TextureFilter::LINEAR);
                depthMapSampler.toggleMipMaps(false);
                depthMapSampler._useRefCompare = true;  //< Use compare function
                depthMapSampler._cmpFunc = ComparisonFunction::LEQUAL;  //< Use less or equal
                TextureDescriptor depthMapDescriptor(TextureType::TEXTURE_CUBE_ARRAY,
                                                     GFXImageFormat::DEPTH_COMPONENT,
                                                     GFXDataFormat::UNSIGNED_INT);
                depthMapDescriptor.setSampler(depthMapSampler);
                depthMapDescriptor.setLayerCount(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS);

                crtTarget = context.allocateRT(RenderTargetUsage::SHADOW, "Cube_ShadowMap");
                crtTarget._rt->addAttachment(depthMapDescriptor, RTAttachment::Type::Depth, 0);
            } break;
        };

        crtTarget._rt->create(resolutions[i], resolutions[i]);

        _depthMapUsage[i].fill(false);
    }
}

void ShadowMap::clearShadowMaps(GFXDevice& context) {
    for (U32 i = 0; i < to_const_uint(ShadowType::COUNT); ++i) {
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i), nullptr);
    }
}

void ShadowMap::bindShadowMaps(GFXDevice& context) {
    for (U8 i = 0; i < to_const_ubyte(ShadowType::COUNT); ++i) {
        RTAttachment::Type attachment
            = static_cast<ShadowType>(i) == ShadowType::LAYERED
                                          ? RTAttachment::Type::Colour
                                          : RTAttachment::Type::Depth;

        U8 bindSlot = LightPool::getShadowBindSlotOffset(static_cast<ShadowType>(i));
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i)).bind(bindSlot, attachment, 0);
    }
}

void ShadowMap::clearShadowMapBuffers(GFXDevice& context) {
    for (U8 i = 0; i < to_const_ubyte(ShadowType::COUNT); ++i) {
        context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, i)).clear(RenderTarget::defaultPolicy());
    }
}

U16 ShadowMap::findDepthMapLayer(ShadowType shadowType) {
    LayerUsageMask& usageMask = _depthMapUsage[to_uint(shadowType)];
    U16 layer = std::numeric_limits<U16>::max();
    for (U16 i = 0; i < to_const_ubyte(Config::Lighting::MAX_SHADOW_CASTING_LIGHTS); ++i) {
        if (usageMask[i] == false) {
            layer = i;
            break;
        }
    }

    return layer;
}

RenderTarget& ShadowMap::getDepthMap() {
    return _context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, to_uint(getShadowMapType())));
}

const RenderTarget& ShadowMap::getDepthMap() const {
    return _context.renderTarget(RenderTargetID(RenderTargetUsage::SHADOW, to_uint(getShadowMapType())));
}

void ShadowMap::commitDepthMapLayer(ShadowType shadowType, U32 layer) {
    DIVIDE_ASSERT(layer < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS,
        "ShadowMap::commitDepthMapLayer error: Insufficient texture layers for shadow mapping");
    LayerUsageMask& usageMask = _depthMapUsage[to_uint(shadowType)];
    usageMask[layer] = true;
}

bool ShadowMap::freeDepthMapLayer(ShadowType shadowType, U32 layer) {
    DIVIDE_ASSERT(layer < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS,
        "ShadowMap::freeDepthMapLayer error: Invalid layer value");
    LayerUsageMask& usageMask = _depthMapUsage[to_uint(shadowType)];
    usageMask[layer] = false;

    return true;
}

vec4<I32> ShadowMap::getViewportForRow(U32 rowIndex) const {
    return vec4<I32>(128, 4 + (128 * rowIndex), 128, 128);
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

ShadowMap* ShadowMapInfo::createShadowMap(GFXDevice& context, const SceneRenderState& renderState, Camera* shadowCamera) {
    if (_shadowMap) {
        return _shadowMap;
    }

    if (!_light->castsShadows()) {
        return nullptr;
    }

    switch (_light->getLightType()) {
        case LightType::POINT: {
            _numLayers = 6;
            _shadowMap = MemoryManager_NEW CubeShadowMap(context, _light, shadowCamera);
        } break;
        case LightType::DIRECTIONAL: {
            DirectionalLight* dirLight = static_cast<DirectionalLight*>(_light);
            _numLayers = dirLight->csmSplitCount();
            _shadowMap = MemoryManager_NEW CascadedShadowMaps(context, _light, shadowCamera, _numLayers);
        } break;
        case LightType::SPOT: {
            _shadowMap = MemoryManager_NEW SingleShadowMap(context, _light, shadowCamera);
        } break;
        default:
            break;
    };

    _shadowMap->init(this);

    return _shadowMap;
}

};
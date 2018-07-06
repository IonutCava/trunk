#include "Headers/ShadowMap.h"
#include "Headers/CubeShadowMap.h"
#include "Headers/SingleShadowMap.h"
#include "Core/Headers/ParamHandler.h"
#include "Scenes/Headers/SceneState.h"
#include "Headers/CascadedShadowMaps.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/RenderTarget/Headers/RenderTarget.h"

namespace Divide {

std::array<RenderTarget*, to_const_uint(ShadowType::COUNT)> ShadowMap::_depthMaps;
std::array<ShadowMap::LayerUsageMask, to_const_uint(ShadowType::COUNT)> ShadowMap::_depthMapUsage;

void ShadowMap::initShadowMaps() {
    std::array<U16, to_const_uint(ShadowType::COUNT)> resolutions;
    resolutions.fill(512);
    if (GFX_DEVICE.shadowDetailLevel() == RenderDetailLevel::ULTRA) {
        // Cap cubemap resolution
        resolutions.fill(2048);
        resolutions[to_const_uint(ShadowType::CUBEMAP)] = 1024;
    } else if (GFX_DEVICE.shadowDetailLevel() == RenderDetailLevel::HIGH) {
        resolutions.fill(1024);
    }

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

                _depthMaps[i] = GFX_DEVICE.newRT();
                _depthMaps[i]->addAttachment(depthMapDescriptor, TextureDescriptor::AttachmentType::Depth);
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

                _depthMaps[i] = GFX_DEVICE.newRT(false);
                _depthMaps[i]->addAttachment(depthMapDescriptor, TextureDescriptor::AttachmentType::Color0);
                _depthMaps[i]->setClearColor(DefaultColors::WHITE());
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

                _depthMaps[i] = GFX_DEVICE.newRT();
                _depthMaps[i]->addAttachment(depthMapDescriptor, TextureDescriptor::AttachmentType::Depth);
            } break;
        };

        _depthMaps[i]->create(resolutions[i], resolutions[i]);
        _depthMapUsage[i].fill(false);
    }
}

void ShadowMap::clearShadowMaps() {
    for (U32 i = 0; i < to_const_uint(ShadowType::COUNT); ++i) {
        MemoryManager::DELETE(_depthMaps[i]);
    }
}

void ShadowMap::bindShadowMaps() {
    for (U8 i = 0; i < to_const_ubyte(ShadowType::COUNT); ++i) {
        TextureDescriptor::AttachmentType attachment
            = static_cast<ShadowType>(i) == ShadowType::LAYERED
                                          ? TextureDescriptor::AttachmentType::Color0
                                          : TextureDescriptor::AttachmentType::Depth;
        _depthMaps[i]->bind(LightPool::getShadowBindSlotOffset(static_cast<ShadowType>(i)), attachment);
    }
}

void ShadowMap::clearShadowMapBuffers() {
    for (U8 i = 0; i < to_const_ubyte(ShadowType::COUNT); ++i) {
        _depthMaps[i]->clear(RenderTarget::defaultPolicy());
    }
}

U32 ShadowMap::findDepthMapLayer(ShadowType shadowType) {
    LayerUsageMask& usageMask = _depthMapUsage[to_uint(shadowType)];
    U32 layer = std::numeric_limits<U32>::max();
    for (U32 i = 0; i < Config::Lighting::MAX_SHADOW_CASTING_LIGHTS; ++i) {
        if (usageMask[i] == false) {
            layer = i;
            break;
        }
    }

    return layer;
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

ShadowMap::ShadowMap(Light* light, Camera* shadowCamera, ShadowType type)
    : _init(false),
      _light(light),
      _shadowCamera(shadowCamera),
      _shadowMapType(type),
      _arrayOffset(0),
      _par(ParamHandler::instance())
{
    _bias.bias();

    _arrayOffset = findDepthMapLayer(_shadowMapType);
    commitDepthMapLayer(_shadowMapType, _arrayOffset);
}

ShadowMap::~ShadowMap()
{
    freeDepthMapLayer(_shadowMapType, _arrayOffset);
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

ShadowMap* ShadowMapInfo::createShadowMap(const SceneRenderState& renderState, Camera* shadowCamera) {
    if (_shadowMap) {
        return _shadowMap;
    }

    if (!_light->castsShadows()) {
        return nullptr;
    }

    switch (_light->getLightType()) {
        case LightType::POINT: {
            _numLayers = 6;
            _shadowMap = MemoryManager_NEW CubeShadowMap(_light, shadowCamera);
        } break;
        case LightType::DIRECTIONAL: {
            DirectionalLight* dirLight = static_cast<DirectionalLight*>(_light);
            _numLayers = dirLight->csmSplitCount();
            _shadowMap = MemoryManager_NEW CascadedShadowMaps(_light, shadowCamera, _numLayers);
        } break;
        case LightType::SPOT: {
            _shadowMap = MemoryManager_NEW SingleShadowMap(_light, shadowCamera);
        } break;
        default:
            break;
    };

    _shadowMap->init(this);

    return _shadowMap;
}

};
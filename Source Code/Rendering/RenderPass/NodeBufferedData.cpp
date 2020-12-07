#include "stdafx.h"

#include "Headers/NodeBufferedData.h"

#include "Platform/Video/Headers/RenderAPIEnums.h"

namespace Divide {
    size_t HashMaterialData(const NodeMaterialData& dataIn) noexcept {
        size_t tempHash = 9999991;
        
        Util::Hash_combine(tempHash, dataIn._albedo.x);
        Util::Hash_combine(tempHash, dataIn._albedo.y);
        Util::Hash_combine(tempHash, dataIn._albedo.z);
        Util::Hash_combine(tempHash, dataIn._albedo.w);

        Util::Hash_combine(tempHash, dataIn._emissiveAndParallax.x);
        Util::Hash_combine(tempHash, dataIn._emissiveAndParallax.y);
        Util::Hash_combine(tempHash, dataIn._emissiveAndParallax.z);
        Util::Hash_combine(tempHash, dataIn._emissiveAndParallax.w);

        Util::Hash_combine(tempHash, dataIn._data.x);
        Util::Hash_combine(tempHash, dataIn._data.y);
        Util::Hash_combine(tempHash, dataIn._data.z);
        Util::Hash_combine(tempHash, dataIn._data.w);
        
        return tempHash;
    }


    size_t HashTexturesData(const NodeMaterialTextures& dataIn) noexcept {
        size_t tempHash = 9999991;
        Util::Hash_combine(tempHash, dataIn._texDiffuse0);
        Util::Hash_combine(tempHash, dataIn._texOpacityMap);
        Util::Hash_combine(tempHash, dataIn._texDiffuse1);
        Util::Hash_combine(tempHash, dataIn._texOMR);
        Util::Hash_combine(tempHash, dataIn._texHeight);
        Util::Hash_combine(tempHash, dataIn._texProjected);
        Util::Hash_combine(tempHash, dataIn._texNormalMap);
        return tempHash;
    }

    [[nodiscard]] SamplerAddress GetTextureAddress(const NodeMaterialTextures& source, const TextureUsage usage) noexcept {
        switch (usage) {
            case TextureUsage::UNIT0: return source._texDiffuse0;
            case TextureUsage::OPACITY: return source._texOpacityMap;
            case TextureUsage::UNIT1: return source._texDiffuse1;
            case TextureUsage::OCCLUSION_METALLIC_ROUGHNESS: return source._texOMR;
            case TextureUsage::HEIGHTMAP: return source._texHeight;
            case TextureUsage::PROJECTION: return source._texProjected;
            case TextureUsage::NORMALMAP: return source._texNormalMap;
        }

        DIVIDE_UNEXPECTED_CALL();
        return 0u;
    }
}; //namespace Divide
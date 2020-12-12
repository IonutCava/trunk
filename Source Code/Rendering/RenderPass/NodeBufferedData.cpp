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
        for (U8 i = 0; i < MATERIAL_TEXTURE_COUNT; ++i) {
            Util::Hash_combine(tempHash, dataIn[i]);
        }

        return tempHash;
    }

}; //namespace Divide
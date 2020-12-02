#include "stdafx.h"

#include "Headers/NodeBufferedData.h"
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
        Util::Hash_combine(tempHash, dataIn._texDiffuse0);
        Util::Hash_combine(tempHash, dataIn._texOpacityMap);
        Util::Hash_combine(tempHash, dataIn._texDiffuse1);
        Util::Hash_combine(tempHash, dataIn._texOMR);
        Util::Hash_combine(tempHash, dataIn._texHeight);
        Util::Hash_combine(tempHash, dataIn._texProjected);
        Util::Hash_combine(tempHash, dataIn._texNormalMap);
        
        return tempHash;
    }
}; //namespace Divide
#include "stdafx.h"

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/PointLight.h"
#include "Rendering/Lighting/Headers/SpotLight.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"

#include "Managers/Headers/SceneManager.h"

namespace Divide {

template<>
CachedResource_ptr ImplResourceLoader<Light>::operator()() {
    void* userPtr = _descriptor.getUserPtr();
    assert(userPtr != nullptr && "ImplResourceLoader error : No target light pool specified!");

    LightPool& pool = *static_cast<LightPool*>(userPtr);

    std::shared_ptr<Light> ptr;
    // descriptor ID is not the same as light ID. This is the light's slot!!
    switch (static_cast<LightType>(_descriptor.getEnumValue())) {
        default:
        case LightType::POINT:
            ptr.reset(MemoryManager_NEW PointLight(_cache, _loadingDescriptorHash, _descriptor.getName(), 1, pool),
                      DeleteResource(_cache));
            break;
        case LightType::DIRECTIONAL:
            ptr.reset(MemoryManager_NEW DirectionalLight(_cache, _loadingDescriptorHash, _descriptor.getName(), pool),
                      DeleteResource(_cache));
            break;
        case LightType::SPOT:
            ptr.reset(MemoryManager_NEW SpotLight(_cache, _loadingDescriptorHash, _descriptor.getName(), 1, pool),
                      DeleteResource(_cache));
            break;
    };
    assert(ptr != nullptr);

    if (!load(ptr, _descriptor.onLoadCallback())) {
        ptr.reset();
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

};

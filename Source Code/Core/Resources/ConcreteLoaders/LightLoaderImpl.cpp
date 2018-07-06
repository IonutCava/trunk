#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/PointLight.h"
#include "Rendering/Lighting/Headers/SpotLight.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"

namespace Divide {

template<>
Light* ImplResourceLoader<Light>::operator()() {
    Light* ptr = nullptr;
    // descriptor ID is not the same as light ID. This is the light's slot!!
    switch (static_cast<LightType>(_descriptor.getEnumValue())) {
        default:
        case LightType::POINT:
            ptr = MemoryManager_NEW PointLight(_descriptor.getName());
            break;
        case LightType::DIRECTIONAL:
            ptr = MemoryManager_NEW DirectionalLight(_descriptor.getName());
            break;
        case LightType::SPOT:
            ptr = MemoryManager_NEW SpotLight(_descriptor.getName());
            break;
    };
    assert(ptr != nullptr);

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

};

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Rendering/Lighting/Headers/PointLight.h"
#include "Rendering/Lighting/Headers/SpotLight.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"

namespace Divide {

Light* ImplResourceLoader<Light>::operator()() {
    Light* ptr = nullptr;
    // descriptor ID is not the same as light ID. This is the light's slot!!
    switch (_descriptor.getEnumValue()) {
        case -1:
        case LIGHT_TYPE_POINT:
        default:
            ptr = MemoryManager_NEW PointLight();
            break;
        case LIGHT_TYPE_DIRECTIONAL:
            ptr = MemoryManager_NEW DirectionalLight();
            break;
        case LIGHT_TYPE_SPOT:
            ptr = MemoryManager_NEW SpotLight();
            break;
    };
    assert(ptr != nullptr);

    if (!ptr->load(_descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        ptr->renderState().useDefaultMaterial(false);
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Light)
};
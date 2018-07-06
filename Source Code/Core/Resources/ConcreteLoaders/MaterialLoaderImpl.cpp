#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Object3D.h"

namespace Divide {

template<>
Material* ImplResourceLoader<Material>::operator()() {
    Material* ptr = MemoryManager_NEW Material(_descriptor.getName());
    assert(ptr != nullptr);

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->setShaderProgram("", true);
        }
        if (_descriptor.getEnumValue() ==
            to_const_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
            ptr->setHardwareSkinning(true);
        }
    }

    return ptr;
}

};

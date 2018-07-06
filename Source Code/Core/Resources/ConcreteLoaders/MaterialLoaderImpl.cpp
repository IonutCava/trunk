#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Object3D.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Material>::operator()() {
    Material_ptr ptr(MemoryManager_NEW Material(_descriptor.getName()), DeleteResource());
    assert(ptr != nullptr);

    if (!load(ptr)) {
        ptr.reset();
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

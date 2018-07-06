#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"

namespace Divide {

template<>
SubMesh* ImplResourceLoader<SubMesh>::operator()() {
    SubMesh* ptr = nullptr;

    if (_descriptor.getEnumValue() ==
        to_const_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)) {
        ptr = MemoryManager_NEW SkinnedSubMesh(_descriptor.getName());
    } else {
        ptr = MemoryManager_NEW SubMesh(_descriptor.getName());
    }

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
        ptr->setID(_descriptor.getID());
    }
    return ptr;
}

};

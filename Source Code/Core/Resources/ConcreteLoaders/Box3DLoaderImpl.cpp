#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"

namespace Divide {

template <>
Box3D* ImplResourceLoader<Box3D>::operator()() {
    D64 size = 1.0;
    if (!_descriptor.getPropertyListString().empty()) {
        size =
            atof(_descriptor.getPropertyListString().c_str());  //<should work
    }

    Box3D* ptr = MemoryManager_NEW Box3D(vec3<F32>(to_float(size)));

    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
    }

    return ptr;
}

};

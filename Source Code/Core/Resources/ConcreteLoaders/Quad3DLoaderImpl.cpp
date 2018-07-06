#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

DEFAULT_LOADER_IMPL(Quad3D)

template<>
Quad3D* ImplResourceLoader<Quad3D>::operator()() {
    Quad3D* ptr = MemoryManager_NEW Quad3D(_descriptor.getMask().b[0] == 0);

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

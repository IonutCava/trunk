#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

Quad3D* ImplResourceLoader<Quad3D>::operator()() {
    Quad3D* ptr = MemoryManager_NEW Quad3D(_descriptor.getMask().b.b0 == 0);

    if (!load(ptr, _descriptor.getName())) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Quad3D)
};
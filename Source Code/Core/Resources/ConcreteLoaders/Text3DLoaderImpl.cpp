#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

namespace Divide {

template<>
Text3D* ImplResourceLoader<Text3D>::operator()() {
    Text3D* ptr = MemoryManager_NEW Text3D(_descriptor.getName(),
                                           _descriptor.getResourceLocation()); //< font

    if (!load(ptr)) {
        MemoryManager::DELETE(ptr);
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
        ptr->setText(_descriptor.getPropertyListString());
    }
    return ptr;
}

};

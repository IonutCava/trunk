#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

namespace Divide {

template<>
Resource_ptr ImplResourceLoader<Text3D>::operator()() {
    std::shared_ptr<Text3D> ptr(MemoryManager_NEW Text3D(_context.gfx(),
                                                         _cache,
                                                         _descriptor.getName(),
                                                         _descriptor.getResourceLocation()),
                                 DeleteResource(_cache)); //< font

    if (!load(ptr)) {
        ptr.reset();
    } else {
        if (_descriptor.getFlag()) {
            ptr->renderState().useDefaultMaterial(false);
        }
        ptr->setText(_descriptor.getPropertyListString());
    }
    return ptr;
}

};

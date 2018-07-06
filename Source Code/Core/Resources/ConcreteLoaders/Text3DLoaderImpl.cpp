#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

namespace Divide {

Text3D* ImplResourceLoader<Text3D>::operator()(){
    Text3D* ptr = MemoryManager_NEW Text3D(_descriptor.getName(), _descriptor.getResourceLocation());

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::DELETE( ptr );
    } else {
        if ( _descriptor.getFlag() ) {
            ptr->renderState().useDefaultMaterial( false );
        }
        ptr->getText() = _descriptor.getPropertyListString();
    }
    return ptr;
}

DEFAULT_LOADER_IMPL(Text3D)

};
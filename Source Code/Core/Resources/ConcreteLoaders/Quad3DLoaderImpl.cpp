#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

namespace Divide {

Quad3D* ImplResourceLoader<Quad3D>::operator()(){
    Quad3D* ptr = New Quad3D(_descriptor.getMask().b.b0 == 0);

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::SAFE_DELETE( ptr );
    } else {
        if ( _descriptor.getFlag() ) {
            ptr->getSceneNodeRenderState().useDefaultMaterial( false );
            ptr->setMaterial( nullptr );
        }
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Quad3D)

};
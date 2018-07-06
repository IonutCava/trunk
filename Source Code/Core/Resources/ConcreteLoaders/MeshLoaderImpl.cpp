#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Importer/Headers/DVDConverter.h"

namespace Divide {

Mesh* ImplResourceLoader<Mesh>::operator()(){
    Mesh* ptr = DVDConverter::getInstance().load(_descriptor.getResourceLocation());

    if ( !load( ptr, _descriptor.getName() ) || !ptr ) {
        MemoryManager::DELETE( ptr );
    } else {
        if ( _descriptor.getFlag() ) {
            ptr->renderState().useDefaultMaterial( false );
        }
        ptr->setResourceLocation( _descriptor.getResourceLocation() );
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Mesh)

};
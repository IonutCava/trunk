#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"

namespace Divide {

SubMesh* ImplResourceLoader<SubMesh>::operator()(){
    SubMesh* ptr = nullptr;

    if ( _descriptor.getEnumValue() == Object3D::OBJECT_FLAG_SKINNED ) {
        ptr = New SkinnedSubMesh( _descriptor.getName() );
    } else {
        ptr = New SubMesh( _descriptor.getName() );
    }

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::SAFE_DELETE( ptr );
    } else {
        if ( _descriptor.getFlag() ) {
            ptr->renderState().useDefaultMaterial( false );
        }
        ptr->setId( _descriptor.getId() );
    }
    return ptr;
}

DEFAULT_LOADER_IMPL(SubMesh)

};
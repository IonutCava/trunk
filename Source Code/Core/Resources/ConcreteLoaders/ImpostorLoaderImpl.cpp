#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

    template<>
    Impostor* ImplResourceLoader<Impostor>::operator()() {
        Impostor* ptr = New Impostor( _descriptor.getName(), 1.0f );

        if ( _descriptor.getFlag() ) {
            ptr->getSceneNodeRenderState().useDefaultMaterial( false );
            ptr->setMaterial( nullptr );
        } else {
            ResourceDescriptor impostorMaterial( "Material_" + _descriptor.getName() );
            ptr->setMaterial( CreateResource<Material>( impostorMaterial ) );
        }

        if ( !load( ptr, _descriptor.getName() ) ) {
            MemoryManager::SAFE_DELETE( ptr );
        }

        return ptr;
    }

    DEFAULT_LOADER_IMPL( Impostor )

};

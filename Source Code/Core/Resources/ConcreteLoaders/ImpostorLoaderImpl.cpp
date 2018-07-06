#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Dynamics/Entities/Headers/Impostor.h"
#include "Geometry/Material/Headers/Material.h"

namespace Divide {

    template<>
    Impostor* ImplResourceLoader<Impostor>::operator()() {
        Impostor* ptr = MemoryManager_NEW Impostor(_descriptor.getName(), 1.0f);

        if ( _descriptor.getFlag() ) {
            ptr->renderState().useDefaultMaterial( false );
        } else {
            Material* matTemp = CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName()));
            matTemp->setShadingMode(Material::SHADING_BLINN_PHONG);
            ptr->setMaterialTpl(matTemp);
        }

        if ( !load( ptr, _descriptor.getName() ) ) {
            MemoryManager::DELETE( ptr );
        }

        return ptr;
    }

    DEFAULT_LOADER_IMPL( Impostor )

};

#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

namespace Divide {

Sphere3D* ImplResourceLoader<Sphere3D>::operator()(){
    Sphere3D* ptr = New Sphere3D(1,32);

    if ( _descriptor.getFlag() ) {
        ptr->renderState().useDefaultMaterial( false );
    } else {
        ptr->setMaterialTpl(CreateResource<Material>(ResourceDescriptor("Material_" + _descriptor.getName())));
    }

    if(!load(ptr,_descriptor.getName())){
        MemoryManager::SAFE_DELETE( ptr );
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Sphere3D)

};
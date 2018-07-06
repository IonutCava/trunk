#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"

Material* ImplResourceLoader<Material>::operator()(){
    Material* ptr = New Material();
    assert(ptr != NULL);

    if(!load(ptr,_descriptor.getName())){
       SAFE_DELETE(ptr);
    }else{
        if(_descriptor.getFlag()) {
            ptr->setShaderProgram("");
        }
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(Material)
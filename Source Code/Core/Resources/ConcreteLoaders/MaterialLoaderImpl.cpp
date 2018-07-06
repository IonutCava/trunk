#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"

Material* ImplResourceLoader<Material>::operator()(){

	Material* ptr = New Material();
	
	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;
	
	if(_descriptor.getFlag()) {
		ptr->setShaderProgram("");
	}

	return ptr;
}

DEFAULT_LOADER_IMPL(Material)
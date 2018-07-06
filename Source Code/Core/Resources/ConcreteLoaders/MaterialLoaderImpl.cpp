#include "Core/Resources/Headers/ResourceLoader.h"
#include "Geometry/Material/Headers/Material.h"

Material* ImplResourceLoader<Material>::operator()(){

	Material* ptr = New Material();
	
	if(!ptr) return NULL;
	if(!ptr->load(_descriptor.getName())) return NULL;
	
	if(_descriptor.getFlag()) {
		ptr->setShaderProgram("");
	}

	return ptr;
}
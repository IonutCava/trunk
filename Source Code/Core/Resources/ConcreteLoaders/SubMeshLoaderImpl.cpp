#include "Core/Resources/Headers/ResourceLoader.h"
#include "Geometry/Shapes/Headers/SubMesh.h"

SubMesh* ImplResourceLoader<SubMesh>::operator()(){

	SubMesh* ptr = New SubMesh(_descriptor.getName());

	if(!ptr->load(_descriptor.getName())) return NULL;
	if(_descriptor.getFlag()){
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}
	ptr->setId(_descriptor.getId());

	return ptr;
}
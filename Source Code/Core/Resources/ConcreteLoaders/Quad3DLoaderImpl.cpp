#include "Core/Resources/Headers/ResourceLoader.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

Quad3D* ImplResourceLoader<Quad3D>::operator()(){

	Quad3D* ptr = New Quad3D();
	
	if(!ptr) return NULL;
	if(!ptr->load(_descriptor.getName())) return NULL;

	if(_descriptor.getFlag()){
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}

	return ptr;
}
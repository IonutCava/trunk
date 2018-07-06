#include "Core/Resources/Headers/ResourceLoader.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"

Sphere3D* ImplResourceLoader<Sphere3D>::operator()(){

	Sphere3D* ptr = New Sphere3D(1,32);

	if(!ptr) return NULL;
	if(!ptr->load(_descriptor.getName())) return NULL;

	if(_descriptor.getFlag()){
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}

	return ptr;
}
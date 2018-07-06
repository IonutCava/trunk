#include "Core/Resources/Headers/ResourceLoader.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

Text3D* ImplResourceLoader<Text3D>::operator()(){

	Text3D* ptr = New Text3D(_descriptor.getName());

	if(!ptr) return NULL;
	if(!ptr->load(_descriptor.getName())) return NULL;
		
	if(_descriptor.getFlag()){
		ptr->useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}

	return ptr;
}
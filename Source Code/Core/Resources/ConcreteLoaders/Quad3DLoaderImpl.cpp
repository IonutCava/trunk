#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"

Quad3D* ImplResourceLoader<Quad3D>::operator()(){

	Quad3D* ptr = New Quad3D();
	
	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;

	if(_descriptor.getFlag()){
		ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}

	return ptr;
}

DEFAULT_LOADER_IMPL(Quad3D)
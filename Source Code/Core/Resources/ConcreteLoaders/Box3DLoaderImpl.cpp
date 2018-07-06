#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"

template<>
Box3D* ImplResourceLoader<Box3D>::operator()(){

	Box3D* ptr = New Box3D(1);

	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;

	if(_descriptor.getFlag()){
		ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}

	return ptr;
}

DEFAULT_LOADER_IMPL(Box3D)
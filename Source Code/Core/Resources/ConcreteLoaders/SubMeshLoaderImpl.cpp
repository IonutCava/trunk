#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"

SubMesh* ImplResourceLoader<SubMesh>::operator()(){

	SubMesh* ptr = NULL;
	if(_descriptor.getEnumValue() == Object3D::OBJECT_FLAG_SKINNED){
		ptr = New SkinnedSubMesh(_descriptor.getName());
	}else{
		ptr = New SubMesh(_descriptor.getName());
	}

	if(!load(ptr,_descriptor.getName())) return NULL;
	if(_descriptor.getFlag()){
		ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}
	ptr->setId(_descriptor.getId());

	return ptr;
}

DEFAULT_LOADER_IMPL(SubMesh)
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

Sphere3D* ImplResourceLoader<Sphere3D>::operator()(){
	Sphere3D* ptr = New Sphere3D(1,32);

	if(_descriptor.getFlag()){
		ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}else{
		ResourceDescriptor sphere3DMaterial("Sphere3DMaterial");
		ptr->setMaterial(CreateResource<Material>(sphere3DMaterial));
	}

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }

	return ptr;
}

DEFAULT_LOADER_IMPL(Sphere3D)
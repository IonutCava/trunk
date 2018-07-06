#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"

Sphere3D* ImplResourceLoader<Sphere3D>::operator()(){

	Sphere3D* ptr = New Sphere3D(1,32);

	if(!ptr) return NULL;

	if(_descriptor.getFlag()){
		ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		ptr->setMaterial(NULL);
	}else{
		ResourceDescriptor sphere3DMaterial("Sphere3DMaterial");
		ptr->setMaterial(CreateResource<Material>(sphere3DMaterial));
	}
	load(ptr,_descriptor.getName());

	return ptr;
}

///Invers winding for Sphere3D culling
template<>
bool ImplResourceLoader<Sphere3D>::load(Sphere3D* const res, const std::string& name){ 
	if(res->getMaterial()){									
	
		RenderStateBlockDescriptor drawState = res->getMaterial()->getRenderState(FINAL_STAGE)->getDescriptor();
		drawState.setCullMode(CULL_MODE_CCW);
		res->getMaterial()->setRenderStateBlock(drawState,FINAL_STAGE);
	}

	return res->setInitialData(name);
}
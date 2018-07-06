#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

Text3D* ImplResourceLoader<Text3D>::operator()(){
	Text3D* ptr = New Text3D(_descriptor.getName(),_descriptor.getResourceLocation());

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
    	if(_descriptor.getFlag()){
	    	ptr->getSceneNodeRenderState().useDefaultMaterial(false);
		    ptr->setMaterial(NULL);
	    }
        ptr->getText() = _descriptor.getPropertyListString();
    }
	return ptr;
}

DEFAULT_LOADER_IMPL(Text3D)
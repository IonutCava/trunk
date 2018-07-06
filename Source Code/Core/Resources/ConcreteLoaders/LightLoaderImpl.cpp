#include "Core/Resources/Headers/ResourceLoader.h"
#include "Rendering/Lighting/Headers/Light.h"

Light* ImplResourceLoader<Light>::operator()(){

	//descriptor ID is not the same as light ID. This is the light's slot!!
	Light* ptr = New Light(_descriptor.getId());

	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;
	ptr->useDefaultMaterial(false);
	ptr->setMaterial(NULL);
	return ptr;
	
}

DEFAULT_LOADER_IMPL(Light)
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Dynamics/Entities/Triggers/Headers/Trigger.h"

Trigger* ImplResourceLoader<Trigger>::operator()(){

	Trigger* ptr = New Trigger();

	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;
	ptr->useDefaultMaterial(false);
	ptr->setMaterial(NULL);

	return ptr;
}

DEFAULT_LOADER_IMPL(Trigger)
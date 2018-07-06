#include "Core/Resources/Headers/ResourceLoader.h"
#include "Environment/Water/Headers/Water.h"

WaterPlane* ImplResourceLoader<WaterPlane>::operator()(){

	WaterPlane* ptr = New WaterPlane();

	assert(ptr != NULL);
	if(!ptr->load(_descriptor.getName())) return NULL;

	return ptr;
}
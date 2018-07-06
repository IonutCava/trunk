#include "Core/Resources/Headers/ResourceLoader.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

TerrainDescriptor* ImplResourceLoader<TerrainDescriptor>::operator()(){

	TerrainDescriptor* ptr = New TerrainDescriptor();

	assert(ptr != NULL);
	if(!load(ptr,_descriptor.getName())) return NULL;

	return ptr;
}

DEFAULT_LOADER_IMPL(TerrainDescriptor)
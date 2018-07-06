#include "Core/Resources/Headers/ResourceLoader.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

TerrainDescriptor* ImplResourceLoader<TerrainDescriptor>::operator()(){

	TerrainDescriptor* ptr = New TerrainDescriptor();

	assert(ptr != NULL);
	if(!ptr->load(_descriptor.getName())) return NULL;

	return ptr;
}
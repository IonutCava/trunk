#include "Core/Resources/Headers/ResourceLoader.h"
#include "Environment/Terrain/Headers/Terrain.h"

Terrain* ImplResourceLoader<Terrain>::operator()() {

	Terrain* ptr = New Terrain();

	assert(ptr != NULL);
	if(!ptr->load(_descriptor.getName())) return NULL;

	return ptr;

}
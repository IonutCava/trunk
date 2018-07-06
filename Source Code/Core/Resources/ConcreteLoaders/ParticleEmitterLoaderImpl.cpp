#include "Core/Resources/Headers/ResourceLoader.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"

ParticleEmitter* ImplResourceLoader<ParticleEmitter>::operator()(){

	ParticleEmitter* ptr = New ParticleEmitter();
	if(!ptr) return NULL;
	if(!load(ptr,_descriptor.getName())) return NULL;
	ptr->useDefaultMaterial(false);
	ptr->setMaterial(NULL);

	return ptr;
}

DEFAULT_LOADER_IMPL(ParticleEmitter)
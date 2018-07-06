#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Audio/Headers/AudioDescriptor.h"

template<>
AudioDescriptor* ImplResourceLoader<AudioDescriptor>::operator()(){

	AudioDescriptor* ptr = New AudioDescriptor();

	assert(ptr != NULL);
	if(!load(ptr,_descriptor.getName())) return NULL;
	ptr->isLooping() = _descriptor.getFlag();

	return ptr;
}

DEFAULT_LOADER_IMPL(AudioDescriptor)
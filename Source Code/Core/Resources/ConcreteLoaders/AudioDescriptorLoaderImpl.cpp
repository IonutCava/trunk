#include "Core/Resources/Headers/ResourceLoader.h"
#include "Hardware/Audio/AudioDescriptor.h"

template<>
AudioDescriptor* ImplResourceLoader<AudioDescriptor>::operator()(){

	AudioDescriptor* ptr = New AudioDescriptor();

	assert(ptr != NULL);
	if(!ptr->load(_descriptor.getName())) return NULL;
	ptr->isLooping() = _descriptor.getFlag();

	return ptr;
}
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Audio/Headers/AudioDescriptor.h"

namespace Divide {

template<>
AudioDescriptor* ImplResourceLoader<AudioDescriptor>::operator()(){
    AudioDescriptor* ptr = MemoryManager_NEW AudioDescriptor(_descriptor.getResourceLocation());
    assert(ptr != nullptr);

    if ( !load( ptr, _descriptor.getName() ) ) {
        MemoryManager::DELETE( ptr );
    } else {
        ptr->isLooping() = _descriptor.getFlag();
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(AudioDescriptor)

};
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Hardware/Audio/Headers/AudioDescriptor.h"

template<>
AudioDescriptor* ImplResourceLoader<AudioDescriptor>::operator()(){
    AudioDescriptor* ptr = New AudioDescriptor(_descriptor.getResourceLocation());
    assert(ptr != nullptr);

    if(!load(ptr,_descriptor.getName())){
        SAFE_DELETE(ptr);
    }else{
        ptr->isLooping() = _descriptor.getFlag();
    }

    return ptr;
}

DEFAULT_LOADER_IMPL(AudioDescriptor)
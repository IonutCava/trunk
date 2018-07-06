#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Audio/Headers/AudioDescriptor.h"

namespace Divide {

template <>
Resource_ptr ImplResourceLoader<AudioDescriptor>::operator()() {
    std::shared_ptr<AudioDescriptor> ptr(MemoryManager_NEW AudioDescriptor(_descriptor.getName(),
                                                                           _descriptor.getResourceLocation()),
                                          DeleteResource());
    if (!load(ptr)) {
        ptr.reset();
    } else {
        ptr->isLooping() = _descriptor.getFlag();
    }

    return ptr;
}

};

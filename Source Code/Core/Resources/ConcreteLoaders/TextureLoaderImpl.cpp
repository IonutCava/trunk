#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

template<>
Texture* ImplResourceLoader<Texture>::operator()() {
    assert(_descriptor.getEnumValue() >= to_const_uint(TextureType::TEXTURE_1D) &&
           _descriptor.getEnumValue() < to_const_uint(TextureType::COUNT));

    Texture* ptr = GFX_DEVICE.newTexture(static_cast<TextureType>(_descriptor.getEnumValue()));

    ptr->enableThreadedLoading(_descriptor.getThreaded());
    ptr->setResourceLocation(_descriptor.getResourceLocation());
    if (_descriptor.getID() > 0) {
        ptr->setNumLayers(_descriptor.getID());
    }
    // Add the specified sampler, if any
    if (_descriptor.hasPropertyDescriptor()) {
        // cast back to a SamplerDescriptor from a PropertyDescriptor
        ptr->setCurrentSampler(
            *_descriptor.getPropertyDescriptor<SamplerDescriptor>());
    }

    if (!load(ptr, _descriptor.getName())) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};

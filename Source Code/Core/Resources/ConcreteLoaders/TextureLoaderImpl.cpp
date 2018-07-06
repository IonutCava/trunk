#include "Core/Headers/Console.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

template<>
Texture* ImplResourceLoader<Texture>::operator()() {
    Texture* ptr = nullptr;

    if (_descriptor.getEnumValue() ==
        to_uint(TextureType::TEXTURE_CUBE_MAP)) {
        ptr = GFX_DEVICE.newTextureCubemap();
    } else if (_descriptor.getEnumValue() ==
                   to_uint(TextureType::TEXTURE_2D_ARRAY) ||
               _descriptor.getEnumValue() ==
                   to_uint(TextureType::TEXTURE_2D_ARRAY_MS)) {
        ptr = GFX_DEVICE.newTextureArray();
        ptr->setNumLayers(to_ubyte(_descriptor.getID()));
    } else {
        ptr = GFX_DEVICE.newTexture2D();
    }

    ptr->enableThreadedLoading(_descriptor.getThreaded());
    ptr->setResourceLocation(_descriptor.getResourceLocation());
    // Add the specified sampler, if any
    if (_descriptor.hasPropertyDescriptor()) {
        // cast back to a SamplerDescriptor from a PropertyDescriptor
        ptr->setCurrentSampler(
            *_descriptor.getPropertyDescriptor<SamplerDescriptor>());
    }

    if (!load(ptr, _descriptor.getName())) {
        Console::errorfn(Locale::get("ERROR_TEXTURE_LOADER_FILE"),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        MemoryManager::DELETE(ptr);
    }

    return ptr;
}

};

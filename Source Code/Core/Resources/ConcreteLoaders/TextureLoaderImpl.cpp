#include "Core/Headers/Console.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {


template<>
CachedResource_ptr ImplResourceLoader<Texture>::operator()() {
    assert(_descriptor.getEnumValue() >= to_base(TextureType::TEXTURE_1D) &&
           _descriptor.getEnumValue() < to_base(TextureType::COUNT));

    assert((!_descriptor.getResourceLocation().empty() && !_descriptor.getResourceName().empty()) ||
            _descriptor.getResourceLocation().empty());

    if (Texture::s_missingTextureFileName == nullptr) {
        Texture::s_missingTextureFileName = "missing_texture.jpg";
    }

    bool threadedLoad = _descriptor.getThreaded();

    TextureType type = static_cast<TextureType>(_descriptor.getEnumValue());
    if ((type == TextureType::TEXTURE_2D_MS || type == TextureType::TEXTURE_2D_ARRAY_MS) && _context.config().rendering.msaaSamples == 0) {
        if (type == TextureType::TEXTURE_2D_MS) {
            type = TextureType::TEXTURE_2D;
        }
        if (type == TextureType::TEXTURE_2D_ARRAY_MS) {
            type = TextureType::TEXTURE_2D_ARRAY;
        }
    }
    stringImpl resourceLocation = _descriptor.getResourceLocation();

    size_t numCommas = std::count(std::cbegin(_descriptor.getResourceName()),
                                  std::cend(_descriptor.getResourceName()),
                                  ',');
    size_t crtNumCommas = std::count(std::cbegin(resourceLocation),
                          std::cend(resourceLocation),
                          ',');
    if (crtNumCommas < numCommas ) {
        if (!resourceLocation.empty()) {
            stringstreamImpl textureLocationList(resourceLocation);
            while (std::getline(textureLocationList, resourceLocation, ',')) {}
        }  else {
            resourceLocation = Paths::g_assetsLocation + Paths::g_texturesLocation;
        }

        for (size_t i = crtNumCommas; i < numCommas; ++i) {
            resourceLocation.append("," + resourceLocation);
        }

        _descriptor.setResourceLocation(resourceLocation);
    }

    Texture_ptr ptr(_context.gfx().newTexture(_loadingDescriptorHash,
                                              _descriptor.getName(),
                                              _descriptor.getResourceName(),
                                              _descriptor.getResourceLocation(),
                                              type,
                                              !_descriptor.getFlag(),
                                              threadedLoad),
                    DeleteResource(_cache));

    if (_descriptor.getID() > 0) {
        ptr->setNumLayers(to_U8(_descriptor.getID()));
    }
    // Add the specified sampler, if any
    if (_descriptor.hasPropertyDescriptor()) {
        // cast back to a SamplerDescriptor from a PropertyDescriptor
        ptr->setCurrentSampler(*_descriptor.getPropertyDescriptor<SamplerDescriptor>());
    }

    if (!load(ptr, _descriptor.onLoadCallback())) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        ptr.reset();
    }

    return ptr;
}

};

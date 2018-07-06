#include "Core/Headers/Console.h"
#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Textures/Headers/Texture.h"
#include "Platform/Video/Textures/Headers/TextureDescriptor.h"

namespace Divide {

namespace {
    stringImpl s_defaultTexturePath;
};

template<>
Resource_ptr ImplResourceLoader<Texture>::operator()() {
    assert(_descriptor.getEnumValue() >= to_const_uint(TextureType::TEXTURE_1D) &&
           _descriptor.getEnumValue() < to_const_uint(TextureType::COUNT));

    assert((!_descriptor.getResourceLocation().empty() && !_descriptor.getResourceName().empty()) ||
            _descriptor.getResourceLocation().empty());

    if (s_defaultTexturePath.empty()) {
        s_defaultTexturePath = _context.entryData().assetsLocation + "/" +
                               _context.config().defaultTextureLocation;
    }

    if (Texture::s_defaultTextureFilePath == nullptr) {
        Texture::s_missingTextureFileName = "missing_texture.jpg";
        Texture::s_defaultTextureFilePath = s_defaultTexturePath.c_str();
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
            resourceLocation = s_defaultTexturePath;
        }

        for (size_t i = crtNumCommas; i < numCommas; ++i) {
            resourceLocation.append("," + s_defaultTexturePath);
        }

        _descriptor.setResourceLocation(resourceLocation);
    }

    Texture_ptr ptr(_context.gfx().newTexture(_descriptor.getName(),
                                              _descriptor.getResourceName(),
                                              _descriptor.getResourceLocation(),
                                              type,
                                              threadedLoad),
                    DeleteResource(_cache));

    if (_descriptor.getID() > 0) {
        ptr->setNumLayers(to_ubyte(_descriptor.getID()));
    }
    // Add the specified sampler, if any
    if (_descriptor.hasPropertyDescriptor()) {
        // cast back to a SamplerDescriptor from a PropertyDescriptor
        ptr->setCurrentSampler(*_descriptor.getPropertyDescriptor<SamplerDescriptor>());
    }

    if (!load(ptr)) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        ptr.reset();
    }

    return ptr;
}

};

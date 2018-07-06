#include "stdafx.h"

#include "Core/Headers/Console.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Headers/XMLEntryData.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Resources/Headers/ResourceLoader.h"
#include "Core/Resources/Headers/ResourceCache.h"

#include "Platform/File/Headers/FileManagement.h"
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

    // Samplers are not optional!
    assert(_descriptor.hasPropertyDescriptor());

    const std::shared_ptr<TextureDescriptor>& texDescriptor = _descriptor.getPropertyDescriptor<TextureDescriptor>();

    if (Texture::s_missingTextureFileName == nullptr) {
        Texture::s_missingTextureFileName = "missing_texture.jpg";
    }

    TextureType type = texDescriptor->type();

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
                                              !_descriptor.getFlag(),
                                              _descriptor.getThreaded(),
                                              *texDescriptor),
                    DeleteResource(_cache));

    if (!load(ptr, _descriptor.onLoadCallback())) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.getResourceLocation().c_str(),
                         _descriptor.getName().c_str());
        ptr.reset();
    }

    return ptr;
}

};

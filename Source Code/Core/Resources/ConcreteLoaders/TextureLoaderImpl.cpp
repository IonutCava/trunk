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
    assert(_descriptor.enumValue() >= to_base(TextureType::TEXTURE_1D) &&
           _descriptor.enumValue() < to_base(TextureType::COUNT));

    // Samplers are not optional!
    assert(_descriptor.hasPropertyDescriptor());

    const std::shared_ptr<TextureDescriptor>& texDescriptor = _descriptor.propertyDescriptor<TextureDescriptor>();

    if (Texture::s_missingTextureFileName == nullptr) {
        Texture::s_missingTextureFileName = "missing_texture.jpg";
    }

    stringImpl resourceLocation = _descriptor.assetLocation().c_str();

    const size_t numCommas = std::count(std::cbegin(_descriptor.assetName()),
                                        std::cend(_descriptor.assetName()),
                                        ',');
    const size_t crtNumCommas = std::count(std::cbegin(resourceLocation),
                                           std::cend(resourceLocation),
                                           ',');

    if (texDescriptor->layerCount() < numCommas + 1) {
        texDescriptor->layerCount(to_U16(numCommas + 1));
    }

    if (crtNumCommas < numCommas ) {

        if (!resourceLocation.empty()) {
            stringstreamImpl textureLocationList(resourceLocation);
            while (std::getline(textureLocationList, resourceLocation, ',')) {}
        }  else {
            resourceLocation = (Paths::g_assetsLocation + Paths::g_texturesLocation.c_str()).c_str();
        }

        for (size_t i = crtNumCommas; i < numCommas; ++i) {
            resourceLocation.append("," + resourceLocation);
        }

        _descriptor.assetLocation(resourceLocation);
    }

    Texture_ptr ptr(_context.gfx().newTexture(_loadingDescriptorHash,
                                              _descriptor.resourceName(),
                                              _descriptor.assetName(),
                                              _descriptor.assetLocation(),
                                              !_descriptor.flag(),
                                              _descriptor.threaded(),
                                              *texDescriptor),
                    DeleteResource(_cache));

    if (!load(ptr)) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOADER_FILE")),
                         _descriptor.assetLocation().c_str(),
                         _descriptor.assetName().c_str(),
                         _descriptor.resourceName().c_str());
        ptr.reset();
    }

    return ptr;
}

};

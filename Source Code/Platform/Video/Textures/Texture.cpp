#include "Headers/Texture.h"

#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Texture::Texture(TextureType type)
    : Resource("temp_texture"),
      _numLayers(1),
      _lockMipMaps(false),
      _samplerDirty(true),
      _mipMapsDirty(true),
      _hasTransparency(false),
      _power2Size(true)
{
    _width = _height = 0;
    _mipMaxLevel = _mipMinLevel = 0;
    _textureData._textureType = type;
    _textureData._samplerHash = _samplerDescriptor.getHash();
}

Texture::~Texture()
{
}

/// Load texture data using the specified file name
bool Texture::load() {
    // Make sure we have a valid file path
    if (getResourceLocation().empty() ||
        getResourceLocation().compare("default") == 0) {
        return false;
    }

    TextureLoadInfo info;
    info._type = _textureData._textureType;

    // Each texture face/layer must be in a comma separated list
    std::stringstream textureLocationList(getResourceLocation());
    // We loop over every texture in the above list and store it in this
    // temporary string
    stringImpl currentTexture;
    while (std::getline(textureLocationList, currentTexture, ',')) {
        // Skip invalid entries
        if (!currentTexture.empty()) {
               // Attempt to load the current entry
                if (!LoadFile(info, currentTexture)) {
                    // Invalid texture files are not handled yet, so stop loading
                    return false;
                }
                info._layerIndex++;
                if (info._type == TextureType::TEXTURE_CUBE_ARRAY) {
                    if (info._layerIndex == 6) {
                        info._layerIndex = 0;
                        info._cubeMapCount++;
                    }
                }
        }
    }

    if (info._type == TextureType::TEXTURE_CUBE_MAP ||
        info._type == TextureType::TEXTURE_CUBE_ARRAY) {
        if (info._layerIndex != 6) {
            Console::errorfn(
                Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"),
                getResourceLocation().c_str());
            return false;
        }
    }

    if (info._type == TextureType::TEXTURE_2D_ARRAY ||
        info._type == TextureType::TEXTURE_2D_ARRAY_MS) {
        if (info._layerIndex != _numLayers) {
            Console::errorfn(
                Locale::get("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT"),
                getResourceLocation().c_str());
            return false;
        }
    }

    if (info._type == TextureType::TEXTURE_CUBE_ARRAY) {
        if (info._cubeMapCount != _numLayers) {
            Console::errorfn(
                Locale::get("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT"),
                getResourceLocation().c_str());
            return false;
        }
    }

    setResourceLocation(_name);

    return Resource::load();
}

/// Use DevIL to load a file into a Texture Object
bool Texture::LoadFile(const TextureLoadInfo& info, const stringImpl& name) {
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(true);
    // Save file contents in  the "img" object
    ImageTools::ImageDataInterface::CreateImageData(name, img);

    // Validate data
    if (!img.data()) {
        Console::errorfn(Locale::get("ERROR_TEXTURE_LOAD"), name.c_str());
        // Missing texture fallback.
        ParamHandler& par = ParamHandler::getInstance();
        img.flip(false);
        // missing_texture.jpg must be something that really stands out
        ImageTools::ImageDataInterface::CreateImageData(
            par.getParam<stringImpl>("assetsLocation", "assets") + "/" +
            par.getParam<stringImpl>("defaultTextureLocation", "textures") +
            "/" + "missing_texture.jpg", img);
    }

    // Extract width, height and bitdepth
    U16 width = img.dimensions().width;
    U16 height = img.dimensions().height;
    // If we have an alpha channel, we must check for translucency
    if (img.alpha()) {
        // Each pixel is independent so this is a brilliant place to parallelize work
        I32 i = 0;
        bool abort = false;
#       pragma omp parallel for
        for (i = 0; i < width; ++i) {
#           pragma omp flush(abort)
            if (!abort) {
                // We process one column per thread
                for (I32 j = 0; j < height; ++j) {
#                  pragma omp flush(abort)
                    if (!abort) {
                        // Check alpha value
                        U8 tempR, tempG, tempB, tempA;
                        img.getColor(i, j, tempR, tempG, tempB, tempA);
                        // If the pixel is transparent, toggle translucency flag
#                       pragma omp critical
                        {
                            // Should be thread-safe
                            if (tempA < 250) {
                               abort = true;
#                              pragma omp flush(abort)
                            }
                        }
                    }
                }
            }
        }
        _hasTransparency = abort;
    }

    // Create a new Rendering API-dependent texture object
    GFXImageFormat internalFormat = GFXImageFormat::RGB8;
    // Select the proper color space internal format
    bool srgb = _samplerDescriptor.srgb();
    // We only support 8 bit per pixel, 1/2/3/4 channel textures
    switch (img.format()) {
        case GFXImageFormat::RED:
            internalFormat = GFXImageFormat::RED8;
            break;
        case GFXImageFormat::RG:
            internalFormat = GFXImageFormat::RG8;
            break;
        case GFXImageFormat::RGB:
            internalFormat =
                srgb ? GFXImageFormat::SRGB8 : 
                       GFXImageFormat::RGB8;
            break;
        case GFXImageFormat::RGBA:
            internalFormat =
                srgb ? GFXImageFormat::SRGBA8 : 
                       GFXImageFormat::RGBA8;
            break;
    }

    U16 mipMaxLevel = 1;
    if (_samplerDescriptor.generateMipMaps()) {
        mipMaxLevel = to_ushort(floorf(log2f(fmaxf(width, height))));
    }
    // Uploading to the GPU dependents on the rendering API
    loadData(info, img.data(), img.dimensions(), vec2<U16>(0, mipMaxLevel),
             img.format(), internalFormat);

    // We will always return true because we load the "missing_texture.jpg" in
    // case of errors
    return true;
}
};

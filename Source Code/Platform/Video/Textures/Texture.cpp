#include "Headers/Texture.h"

#include "Utility/Headers/ImageTools.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Texture::Texture(TextureType type, const bool flipped)
    : HardwareResource("temp_texture"),
      _flipped(flipped),
      _bitDepth(0),
      _numLayers(1),
      _samplerDirty(true),
      _mipMapsDirty(true),
      _hasTransparency(false),
      _power2Size(true)
{
    _width = _height = 0;
    _textureData._textureType = type;
    _textureData._samplerHash = _samplerDescriptor.getHash();
}

Texture::~Texture()
{
}

/// Load texture data using the specified file name
bool Texture::generateHWResource(const stringImpl& name) {
    // Make sure we have a valid file path
    if (getResourceLocation().empty() ||
        getResourceLocation().compare("default") == 0) {
        return false;
    }
    // Each texture type is loaded differently
    if (_textureData._textureType == TextureType::TEXTURE_2D) {
        // 2D Textures are loaded directly
        if (!LoadFile(to_uint(_textureData._textureType), getResourceLocation())) {
            return false;
        }
        // Cube maps and texture arrays need to load each face/layer separately
    } else if (_textureData._textureType == TextureType::TEXTURE_CUBE_MAP ||
               _textureData._textureType == TextureType::TEXTURE_2D_ARRAY) {
        // Each texture face/layer must be in a comma separated list
        std::stringstream textureLocationList(getResourceLocation().c_str());
        // We loop over every texture in the above list and store it in this
        // temporary string
        std::string currentTexture;
        U8 idx = 0;
        while (std::getline(textureLocationList, currentTexture, ',')) {
            // Skip invalid entries
            if (!currentTexture.empty()) {
                // Attempt to load the current entry
                if (!LoadFile(idx, stringAlg::toBase(currentTexture))) {
                    // Invalid texture files are not handled yet, so stop
                    // loading
                    return false;
                }
                idx++;
            }
        }
        // Cube maps have exactly 6 faces, so make sure all of them are loaded
        if (idx != 6 && _textureData._textureType == TextureType::TEXTURE_CUBE_MAP) {
            Console::errorfn(
                Locale::get("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT"),
                getResourceLocation().c_str());
            return false;
        }
        // Same logic applies to texture arrays
        if (idx != _numLayers &&
            _textureData._textureType == TextureType::TEXTURE_2D_ARRAY) {
            Console::errorfn(
                Locale::get("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT"),
                getResourceLocation().c_str());
            return false;
        }
    } else {
        // We don't support other texture types
        return false;
    }
    setResourceLocation(name);

    return HardwareResource::generateHWResource(name);
}

/// Use DevIL to load a file into a Texture Object
bool Texture::LoadFile(U32 target, const stringImpl& name) {
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(_flipped);
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
    _bitDepth = img.bpp();
    // If we have an alpha channel, we must check for translucency
    if (img.alpha()) {
        // Each pixel is independent so this is a brilliant place to parallelize
        // work
        bool abort = false;
#pragma omp parallel for
        for (I32 i = 0; i < width; i++) {
#pragma omp flush(abort)
            if (!abort) {
                // We process one column per thread
                for (I32 j = 0; j < height; j++) {
                    // Check alpha value
                    if (img.getColor(i, j).a < 250) {
// If the pixel is transparent, toggle translucency flag
#pragma omp critical
                        {
                            // Should be thread-safe
                            _hasTransparency = true;
                            abort = true;
#pragma omp flush(abort)
                        }
                    }
                }
            }
        }
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
        mipMaxLevel = static_cast<U16>(floorf(log2f(fmaxf(width, height))));
    }
    // Uploading to the GPU dependents on the rendering API
    loadData(target, img.data(), img.dimensions(), vec2<U16>(0, mipMaxLevel),
             img.format(), internalFormat);

    // We will always return true because we load the "missing_texture.jpg" in
    // case of errors
    return true;
}
};
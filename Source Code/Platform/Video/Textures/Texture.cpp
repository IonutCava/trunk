#include "Headers/Texture.h"

#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

Texture::Texture(GFXDevice& context, TextureType type, bool asyncLoad)
    : GraphicsResource(context),
      Resource("temp_texture"),
      _numLayers(1),
      _lockMipMaps(false),
      _samplerDirty(true),
      _mipMapsDirty(true),
      _hasTransparency(false),
      _asyncLoad(asyncLoad)
{
    _width = _height = 0;
    _mipMaxLevel = _mipMinLevel = 0;
    _textureData._textureType = type;
    _textureData._samplerHash = _descriptor._samplerDescriptor.getHash();
}

Texture::~Texture()
{
}

bool Texture::load() {
    assert(!getName().empty());

    _context.loadInContext(_asyncLoad ? CurrentContext::GFX_LOADING_CTX
                                      : CurrentContext::GFX_RENDERING_CTX,
        [&]() {
            threadedLoad(getName());
        }
    );

    return true;
}

/// Load texture data using the specified file name
void Texture::threadedLoad(const stringImpl& name) {
    // Make sure we have a valid file path
    if (!getResourceLocation().empty() && getResourceLocation().compare("default") != 0) {

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
                        return;
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
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }

        if (info._type == TextureType::TEXTURE_2D_ARRAY ||
            info._type == TextureType::TEXTURE_2D_ARRAY_MS) {
            if (info._layerIndex != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }

        if (info._type == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._cubeMapCount != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }
    }

    setResourceLocation(_name);

    Resource::load();
}

bool Texture::LoadFile(const TextureLoadInfo& info, const stringImpl& name) {
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(true);
    // Save file contents in  the "img" object
    ImageTools::ImageDataInterface::CreateImageData(name, img);

    // Validate data
    if (!img.data()) {
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOAD")), name.c_str());
        // Missing texture fallback.
        ParamHandler& par = ParamHandler::getInstance();
        img.flip(false);
        // missing_texture.jpg must be something that really stands out
        ImageTools::ImageDataInterface::CreateImageData(
            par.getParam<stringImpl>(_ID("assetsLocation"), "assets") + "/" +
            par.getParam<stringImpl>(_ID("defaultTextureLocation"), "textures") +
            "/" + "missing_texture.jpg", img);
    }
    
    // Extract width, height and bitdepth
    U16 width = img.dimensions().width;
    U16 height = img.dimensions().height;
    // If we have an alpha channel, we must check for translucency
    if (img.alpha()) {
        // Each pixel is independent so this is a brilliant place to parallelize work
        I32 i = 0;
        ACKNOWLEDGE_UNUSED(i);
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
    _descriptor._type = info._type;
    _descriptor._internalFormat = GFXImageFormat::RGB8;
    // Select the proper color space internal format
    bool srgb = _descriptor._samplerDescriptor.srgb();
    // We only support 8 bit per pixel, 1/2/3/4 channel textures
    switch (img.format()) {
        case GFXImageFormat::LUMINANCE:
            _descriptor._internalFormat = GFXImageFormat::LUMINANCE;
            break;
        case GFXImageFormat::RED:
            _descriptor._internalFormat = GFXImageFormat::RED8;
            break;
        case GFXImageFormat::RG:
            _descriptor._internalFormat = GFXImageFormat::RG8;
            break;
        case GFXImageFormat::BGR:
        case GFXImageFormat::RGB:
            _descriptor._internalFormat =
                srgb ? GFXImageFormat::SRGB8 : 
                       GFXImageFormat::RGB8;
            break;
        case GFXImageFormat::BGRA:
        case GFXImageFormat::RGBA:
            _descriptor._internalFormat =
                srgb ? GFXImageFormat::SRGBA8 : 
                       GFXImageFormat::RGBA8;
            break;
        case GFXImageFormat::COMPRESSED_RGB_DXT1: {
            _descriptor._internalFormat = 
                srgb ? GFXImageFormat::COMPRESSED_SRGB_DXT1 :
                       GFXImageFormat::COMPRESSED_RGB_DXT1;
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT1: {
            _descriptor._internalFormat =
                srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT1 :
                       GFXImageFormat::COMPRESSED_RGBA_DXT1;
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT3: {
            _descriptor._internalFormat =
                srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT3 :
                       GFXImageFormat::COMPRESSED_RGBA_DXT3;
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT5: {
            _descriptor._internalFormat =
                srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT5 :
                       GFXImageFormat::COMPRESSED_RGBA_DXT5;
            _descriptor._compressed = true;
        } break;
    }

    U16 mipMaxLevel = to_ushort(img.mipCount());
    
    if (!_descriptor._compressed) {
        if (_descriptor._samplerDescriptor.generateMipMaps() && mipMaxLevel == 0) {
            mipMaxLevel = to_ushort(floorf(log2f(fmaxf(width, height))));
        } else {
            mipMaxLevel += 1;
        }
    }
    // Uploading to the GPU dependents on the rendering API
    loadData(info,
             _descriptor,
             img.imageLayers(),
             vec2<U16>(0, mipMaxLevel));

    // We will always return true because we load the "missing_texture.jpg" in
    // case of errors
    return true;
}
};

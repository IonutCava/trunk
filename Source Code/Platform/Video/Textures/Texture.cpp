#include "Headers/Texture.h"

#include "Core/Headers/TaskPool.h"
#include "Core/Headers/ParamHandler.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

namespace {
    static const U16 g_partitionSize = 128;
};

Texture::Texture(GFXDevice& context,
                 const stringImpl& name,
                 const stringImpl& resourceLocation,
                 TextureType type,
                 bool asyncLoad)
    : GraphicsResource(context),
      Resource(name, resourceLocation),
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
    _context.loadInContext(_asyncLoad ? CurrentContext::GFX_LOADING_CTX
                                      : CurrentContext::GFX_RENDERING_CTX,
        [&](const std::atomic_bool& stopRequested) {
            threadedLoad();
        }
    );

    return true;
}

/// Load texture data using the specified file name
void Texture::threadedLoad() {
    // Make sure we have a valid file path
    if (!getResourceLocation().empty() && getResourceLocation().compare("default") != 0) {

        TextureLoadInfo info;
        info._type = _textureData._textureType;

        // Each texture face/layer must be in a comma separated list
        stringstreamImpl textureLocationList(getResourceLocation());
        // We loop over every texture in the above list and store it in this
        // temporary string
        stringImpl currentTexture;
        while (std::getline(textureLocationList, currentTexture, ',')) {
            // Skip invalid entries
            if (!currentTexture.empty()) {
                   // Attempt to load the current entry
                    if (!loadFile(info, currentTexture)) {
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

    Resource::load();
}

bool Texture::loadFile(const TextureLoadInfo& info, const stringImpl& name) {
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
        ParamHandler& par = ParamHandler::instance();
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
        // should this be atomic? At most, we run an extra task -Ionut
        std::atomic_bool abort = false;

        auto findAlpha = [&abort, &img, height](const std::atomic_bool& stopRequested, U16 start, U16 end) {
            U8 tempR, tempG, tempB, tempA;
            for (U16 i = start; i < end; ++i) {
                for (I32 j = 0; j < height; ++j) {
                    if (!abort) {
                        img.getColour(i, j, tempR, tempG, tempB, tempA);
                        if (tempA < 250) {
                            abort = true;
                        }
                    }
                }
                if (stopRequested) {
                    break;
                }
            }
        };

        parallel_for(findAlpha, width, g_partitionSize);

        _hasTransparency = abort;
    }

    Console::d_printfn(Locale::get(_ID("TEXTURE_HAS_TRANSPARENCY")),
                       name.c_str(),
                       _hasTransparency ? "yes" : "no");

    // Create a new Rendering API-dependent texture object
    _descriptor._type = info._type;
    _descriptor._internalFormat = GFXImageFormat::RGB8;
    // Select the proper colour space internal format
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
        if (_descriptor._samplerDescriptor.generateMipMaps() && mipMaxLevel <= 1) {
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

    // We will always return true because we load the "missing_texture.jpg" in case of errors
    return true;
}
};

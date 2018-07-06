#include "stdafx.h"

#include "Headers/Texture.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

namespace {
    static const U16 g_partitionSize = 128;
};

const char* Texture::s_missingTextureFileName = nullptr;

Texture::Texture(GFXDevice& context,
                 size_t descriptorHash,
                 const stringImpl& name,
                 const stringImpl& resourceName,
                 const stringImpl& resourceLocation,
                 TextureType type,
                 bool isFlipped,
                 bool asyncLoad)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, name, resourceName, resourceLocation),
      GraphicsResource(context, getGUID()),
      _numLayers(1),
      _lockMipMaps(false),
      _samplerDirty(true),
      _mipMapsDirty(true),
      _hasTransparency(false),
      _flipped(isFlipped),
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

bool Texture::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    _context.loadInContext(_asyncLoad ? CurrentContext::GFX_LOADING_CTX
                                      : CurrentContext::GFX_RENDERING_CTX,
        [this, onLoadCallback](const Task& parent) {
            threadedLoad(std::move(onLoadCallback));
        }
    );

    return true;
}

/// Load texture data using the specified file name
void Texture::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {
    TextureLoadInfo info;
    info._type = _textureData._textureType;

    // Each texture face/layer must be in a comma separated list
    stringstreamImpl textureLocationList(getResourceLocation());
    stringstreamImpl textureFileList(getResourceName());

    bool loadFromFile = false;
    // We loop over every texture in the above list and store it in this
    // temporary string
    stringImpl currentTextureFile;
    stringImpl currentTextureLocation;
    stringImpl currentTextureFullPath;
    while (std::getline(textureLocationList, currentTextureLocation, ',') &&
           std::getline(textureFileList, currentTextureFile, ','))
    {
        loadFromFile = true;
        Util::Trim(currentTextureFile);
        // Skip invalid entries
        if (!currentTextureFile.empty()) {
            currentTextureFullPath = (currentTextureLocation.empty() ? Paths::g_texturesLocation
                                                                     : currentTextureLocation) +
                                     "/" +
                                     currentTextureFile;

                // Attempt to load the current entry
                if (!loadFile(info, currentTextureFullPath)) {
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

    if (loadFromFile) {
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
}

bool Texture::loadFile(const TextureLoadInfo& info, const stringImpl& name) {
    // Create a new imageData object
    ImageTools::ImageData img;
    // Flip image if needed
    img.flip(_flipped);
    // Save file contents in  the "img" object
    ImageTools::ImageDataInterface::CreateImageData(name, img);

    // Validate data
    if (!img.data()) {
        if (info._layerIndex > 0) {
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LAYER_LOAD")), name.c_str());
            return false;
        }
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOAD")), name.c_str());
        // Missing texture fallback.
        img.flip(false);
        // missing_texture.jpg must be something that really stands out
        ImageTools::ImageDataInterface::CreateImageData(Paths::g_assetsLocation + Paths::g_texturesLocation + s_missingTextureFileName, img);

    }
    
    // Extract width, height and bitdepth
    U16 width = img.dimensions().width;
    U16 height = img.dimensions().height;
    // If we have an alpha channel, we must check for translucency
    if (img.alpha()) {
        auto findAlpha = [this, &img, height](const Task& parent, U32 start, U32 end) {
            U8 tempR, tempG, tempB, tempA;
            for (U32 i = start; i < end; ++i) {
                if (!_hasTransparency) {
                    for (I32 j = 0; j < height; ++j) {
                        img.getColour(i, j, tempR, tempG, tempB, tempA);
                        if (tempA < 250) {
                            _hasTransparency = true;
                             return;
                        }
                    }
                }
                if (parent.stopRequested()) {
                    break;
                }
            }
        };

        parallel_for(findAlpha, width, g_partitionSize);
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
            assert(!srgb);
            _descriptor._internalFormat = GFXImageFormat::LUMINANCE;
            break;
        case GFXImageFormat::RED:
            assert(!srgb);
            _descriptor._internalFormat = GFXImageFormat::RED8;
            break;
        case GFXImageFormat::RG:
            assert(!srgb);
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
                srgb ? GFXImageFormat::SRGB_ALPHA8 :
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

    U16 mipMaxLevel = to_U16(img.mipCount());
    
    if (!_descriptor._compressed) {
        if (_descriptor._samplerDescriptor.generateMipMaps()) {
            if (mipMaxLevel <= 1) {
                mipMaxLevel = computeMipCount(width, height);
            }

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

U16 Texture::computeMipCount(U16 width, U16 height) {
    return to_U16(std::floorf(std::log2f(std::fmaxf(to_F32(width), to_F32(height)))));
}
};

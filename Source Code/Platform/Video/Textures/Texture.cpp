#include "stdafx.h"

#include "Headers/Texture.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
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
                 bool isFlipped,
                 bool asyncLoad,
                 const TextureDescriptor& texDescriptor)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, name, resourceName, resourceLocation),
      GraphicsResource(context, GraphicsResource::Type::TEXTURE, getGUID()),
      _descriptor(texDescriptor),
      _numLayers(texDescriptor._layerCount),
      _hasTransparency(false),
      _hasTranslucency(false),
      _flipped(isFlipped),
      _asyncLoad(asyncLoad)
{
    if (_descriptor.msaaSamples() == -1) {
        _descriptor.msaaSamples(0);
    }

    if (_descriptor.msaaSamples() == 0) {
        if (_descriptor.type() == TextureType::TEXTURE_2D_MS) {
            _descriptor.type(TextureType::TEXTURE_2D);
        }
        if (_descriptor.type() == TextureType::TEXTURE_2D_ARRAY_MS) {
            _descriptor.type(TextureType::TEXTURE_2D_ARRAY);
        }
    }

    _width = _height = 0;
    _textureData._textureType = _descriptor.type();
}

Texture::~Texture()
{
}

bool Texture::load(const DELEGATE_CBK<void, CachedResource_wptr>& onLoadCallback) {
    CreateTask(_context.parent().platformContext(),
               [this, onLoadCallback](const Task& parent) {
                    threadedLoad(std::move(onLoadCallback));
              }).startTask(_asyncLoad ? TaskPriority::DONT_CARE : TaskPriority::REALTIME);

    return true;
}

/// Load texture data using the specified file name
void Texture::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {
    TextureLoadInfo info;

    // Each texture face/layer must be in a comma separated list
    stringstreamImpl textureLocationList(getResourceLocation());
    stringstreamImpl textureFileList(getResourceName());

    bool loadFromFile = false;

    vector<stringImpl> fileNames;

    // We loop over every texture in the above list and store it in this
    // temporary string
    stringImpl currentTextureFile;
    stringImpl currentTextureLocation;
    stringImpl currentTextureFullPath;
    while (std::getline(textureLocationList, currentTextureLocation, ',') &&
           std::getline(textureFileList, currentTextureFile, ','))
    {
        Util::Trim(currentTextureFile);

        // Skip invalid entries
        if (!currentTextureFile.empty()) {
            fileNames.push_back(
                (currentTextureLocation.empty() ? Paths::g_texturesLocation
                                                : currentTextureLocation) +
                "/" +
                currentTextureFile);
        }
    }

    loadFromFile = !fileNames.empty();

    hashMap<U64, ImageTools::ImageData> dataStorage;

    for(const stringImpl& file : fileNames) {
        // Attempt to load the current entry
        if (!loadFile(info, file, dataStorage[_ID(file.c_str())])) {
            // Invalid texture files are not handled yet, so stop loading
            return;
        }
        info._layerIndex++;
        if (_textureData._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._layerIndex == 6) {
                info._layerIndex = 0;
                info._cubeMapCount++;
            }
        }
    }

    if (loadFromFile) {
        if (_textureData._textureType == TextureType::TEXTURE_CUBE_MAP ||
            _textureData._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._layerIndex != 6) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }

        if (_textureData._textureType == TextureType::TEXTURE_2D_ARRAY ||
            _textureData._textureType == TextureType::TEXTURE_2D_ARRAY_MS) {
            if (info._layerIndex != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }

        if (_textureData._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._cubeMapCount != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    getResourceLocation().c_str());
                return;
            }
        }
    }
}

bool Texture::loadFile(const TextureLoadInfo& info, const stringImpl& name, ImageTools::ImageData& fileData) {
    // If we haven't already loaded this file, do so
    if (!fileData.data()) {
        // Flip image if needed
        fileData.flip(_flipped);
        // Save file contents in  the "img" object
        ImageTools::ImageDataInterface::CreateImageData(name, fileData);

        // Validate data
        if (!fileData.data()) {
            if (info._layerIndex > 0) {
                Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LAYER_LOAD")), name.c_str());
                return false;
            }
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOAD")), name.c_str());
            // Missing texture fallback.
            fileData.flip(false);
            // missing_texture.jpg must be something that really stands out
            ImageTools::ImageDataInterface::CreateImageData(Paths::g_assetsLocation + Paths::g_texturesLocation + s_missingTextureFileName, fileData);

        }

        // Extract width, height and bitdepth
        U16 width = fileData.dimensions().width;
        U16 height = fileData.dimensions().height;
        // If we have an alpha channel, we must check for translucency/transparency

        if (fileData.alpha()) {
            auto findAlpha = [this, &fileData, height](const Task& parent, U32 start, U32 end) {
                U8 tempA;
                for (U32 i = start; i < end; ++i) {
                    for (I32 j = 0; j < height; ++j) {
                        if (_hasTransparency && _hasTranslucency) {
                            return;
                        }
                        fileData.getAlpha(i, j, tempA);
                        if (IS_IN_RANGE_INCLUSIVE(tempA, 0, 254)) {
                            _hasTransparency = true;
                            _hasTranslucency = tempA > 1;
                            if (_hasTranslucency) {
                                return;
                            }
                        }
                    }
                    if (StopRequested(&parent)) {
                        break;
                    }
                }
            };

            parallel_for(_context.parent().platformContext(), findAlpha, width, g_partitionSize);
        }

        Console::d_printfn(Locale::get(_ID("TEXTURE_HAS_TRANSPARENCY_TRANSLUCENCY")),
                                        name.c_str(),
                                        _hasTransparency ? "yes" : "no",
                                        _hasTranslucency ? "yes" : "no");
    }

    // Create a new Rendering API-dependent texture object
    if (info._layerIndex == 0) {
        _descriptor._type = _textureData._textureType;
        _descriptor._mipLevels.max = to_U16(fileData.mipCount());

        _descriptor.internalFormat(fileData.format());
    } else {
        DIVIDE_ASSERT(_descriptor._type == _textureData._textureType, "Texture::loadFile error: Texture Layer with different type detected!");
    }
    // Uploading to the GPU dependents on the rendering API
    loadData(info, fileData.imageLayers());
    
    // We will always return true because we load the "missing_texture.jpg" in case of errors
    return true;
    
}

void Texture::setCurrentSampler(const SamplerDescriptor& descriptor) {
    // This can be called at any time
    _descriptor.setSampler(descriptor);
}

void Texture::validateDescriptor() {
    // DEPTH_COMPONENT defaults to DEPTH_COMPONENT32
    if (_descriptor.internalFormat() == GFXImageFormat::DEPTH_COMPONENT) {
        _descriptor.internalFormat(GFXImageFormat::DEPTH_COMPONENT32);
    }

    // Select the proper colour space internal format
    bool srgb = _descriptor._samplerDescriptor.srgb();
    // We only support 8 bit per pixel, 1/2/3/4 channel textures
    switch (_descriptor.internalFormat()) {
        case GFXImageFormat::LUMINANCE: {
            assert(!srgb);
            _descriptor.internalFormat(GFXImageFormat::LUMINANCE);
        }break;
        case GFXImageFormat::RED: {
            assert(!srgb);
            _descriptor.internalFormat(GFXImageFormat::RED8);
        }break;
        case GFXImageFormat::RG: {
            assert(!srgb);
            _descriptor.internalFormat(GFXImageFormat::RG8);
        } break;
        case GFXImageFormat::BGR:
        case GFXImageFormat::RGB: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::SRGB8 : GFXImageFormat::RGB8);
        }break;
        case GFXImageFormat::BGRA:
        case GFXImageFormat::RGBA: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::SRGB_ALPHA8 : GFXImageFormat::RGBA8);
        }break;
        case GFXImageFormat::COMPRESSED_RGB_DXT1: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::COMPRESSED_SRGB_DXT1 :
                                              GFXImageFormat::COMPRESSED_RGB_DXT1);
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT1: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT1 :
                                              GFXImageFormat::COMPRESSED_RGBA_DXT1);
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT3: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT3 :
                                              GFXImageFormat::COMPRESSED_RGBA_DXT3);
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT5: {
            _descriptor.internalFormat(srgb ? GFXImageFormat::COMPRESSED_SRGB_ALPHA_DXT5 :
                                              GFXImageFormat::COMPRESSED_RGBA_DXT5);
            _descriptor._compressed = true;
        } break;
    }

    if (!_descriptor._compressed) {
        if (_descriptor._samplerDescriptor.generateMipMaps()) {
            if (_descriptor._mipLevels.max <= 1) {
                _descriptor._mipLevels.max = computeMipCount(_width, _height) + 1;
            }
        }
    }

    _textureData._textureType = _descriptor.type();
}

U16 Texture::computeMipCount(U16 width, U16 height) {
    return to_U16(std::floorf(std::log2f(std::fmaxf(to_F32(width), to_F32(height)))));
}
};

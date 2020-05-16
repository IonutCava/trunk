#include "stdafx.h"

#include "Headers/Texture.h"

#include "Core/Headers/Kernel.h"
#include "Core/Headers/ByteBuffer.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/EngineTaskPool.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

const char* Texture::s_missingTextureFileName = nullptr;

Texture::Texture(GFXDevice& context,
                 size_t descriptorHash,
                 const Str128& name,
                 const stringImpl& assetNames,
                 const stringImpl& assetLocations,
                 bool isFlipped,
                 bool asyncLoad,
                 const TextureDescriptor& texDescriptor)
    : CachedResource(ResourceType::GPU_OBJECT, descriptorHash, name, assetNames, assetLocations),
      GraphicsResource(context, GraphicsResource::Type::TEXTURE, getGUID(), _ID(name.c_str())),
      _data{0u, 0u, TextureType::COUNT},
      _descriptor(texDescriptor),
      _numLayers(texDescriptor._layerCount),
      _hasTransparency(false),
      _hasTranslucency(false),
      _flipped(isFlipped),
      _asyncLoad(asyncLoad)
{
    _width = _height = 0;
}

bool Texture::load() {
    if (_asyncLoad) {
        Start(*CreateTask(_context.context().taskPool(TaskPoolType::HIGH_PRIORITY),
            [this](const Task & parent) {
                threadedLoad();
        }));
    } else {
        threadedLoad();
    }
    return true;
}

/// Load texture data using the specified file name
void Texture::threadedLoad() {
    OPTICK_EVENT();

    // Each texture face/layer must be in a comma separated list
    stringstreamImpl textureLocationList(assetLocation());
    stringstreamImpl textureFileList(assetName().c_str());

    _descriptor._sourceFileList.reserve(6);

    ImageTools::ImageData dataStorage = {};
    // Flip image if needed
    dataStorage.flip(_flipped);
    dataStorage.set16Bit(_descriptor.dataType() == GFXDataFormat::FLOAT_16 ||
                         _descriptor.dataType() == GFXDataFormat::SIGNED_SHORT ||
                         _descriptor.dataType() == GFXDataFormat::UNSIGNED_SHORT);

    bool loadedFromFile = false;
    // We loop over every texture in the above list and store it in this temporary string
    stringImpl currentTextureFile;
    stringImpl currentTextureLocation;
    stringImpl currentTextureFullPath;
    while (std::getline(textureLocationList, currentTextureLocation, ',') &&
           std::getline(textureFileList, currentTextureFile, ','))
    {
        Util::Trim(currentTextureFile);

        // Skip invalid entries
        if (!currentTextureFile.empty()) {

            currentTextureFullPath = (currentTextureLocation.empty() ? Paths::g_texturesLocation.c_str() : currentTextureLocation);
            currentTextureFullPath.append("/" + currentTextureFile);

            _descriptor._sourceFileList.push_back(currentTextureFile);

            // Attempt to load the current entry
            if (!loadFile(currentTextureFullPath, dataStorage)) {
                // Invalid texture files are not handled yet, so stop loading
                continue;
            }

            loadedFromFile = true;
        }
    }
    _descriptor._sourceFileList.shrink_to_fit();

    if (loadedFromFile) {
        // Create a new Rendering API-dependent texture object
        _descriptor._mipLevels.max = to_U16(dataStorage.mipCount());
        _descriptor._mipCount = _descriptor.mipLevels().max;
        _descriptor._compressed = dataStorage.compressed();
        _descriptor.baseFormat(dataStorage.format());
        _descriptor.dataType(dataStorage.dataType());
        // Uploading to the GPU dependents on the rendering API
        loadData(dataStorage);

        if (_descriptor.type() == TextureType::TEXTURE_CUBE_MAP ||
            _descriptor.type() == TextureType::TEXTURE_CUBE_ARRAY) {
            if (dataStorage.layerCount() % 6 != 0) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT")),
                    resourceName().c_str());
                return;
            }
        }

        if (_descriptor.type() == TextureType::TEXTURE_2D_ARRAY ||
            _descriptor.type() == TextureType::TEXTURE_2D_ARRAY_MS) {
            if (dataStorage.layerCount() != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    resourceName().c_str());
                return;
            }
        }

        if (_descriptor.type() == TextureType::TEXTURE_CUBE_ARRAY) {
            if (dataStorage.layerCount() / 6 != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    resourceName().c_str());
                return;
            }
        }
    }
}

bool Texture::loadFile(const stringImpl& name, ImageTools::ImageData& fileData) {

    if (!ImageTools::ImageDataInterface::CreateImageData(name, _width, _height, _descriptor.srgb(), fileData)) {
        if (fileData.layerCount() > 0) {
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LAYER_LOAD")), name.c_str());
            return false;
        }
        Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOAD")), name.c_str());
        // Missing texture fallback.
        fileData.flip(false);
        // missing_texture.jpg must be something that really stands out
        ImageTools::ImageDataInterface::CreateImageData(((Paths::g_assetsLocation + Paths::g_texturesLocation) + s_missingTextureFileName).c_str(), _width, _height, _descriptor.srgb(), fileData);
    } else {
        return checkTransparency(name, fileData);
    }

    return true;
}

bool Texture::checkTransparency(const stringImpl& name, ImageTools::ImageData& fileData) {
    constexpr std::array<std::string_view, 2> searchPattern = {
        "//", "\\"
    };

    const U32 layer = to_U32(fileData.layerCount() - 1);

    // Extract width, height and bitdepth
    const U16 width = fileData.dimensions(layer, 0u).width;
    const U16 height = fileData.dimensions(layer, 0u).height;
    // If we have an alpha channel, we must check for translucency/transparency

    FileWithPath fwp = splitPathToNameAndLocation(name.c_str());
    Util::ReplaceStringInPlace(fwp._path, searchPattern, "/");
    Util::ReplaceStringInPlace(fwp._path, "/", "_");
    if (fwp._path.back() == '_') {
        fwp._path.pop_back();
    }
    const Str256 cachePath = Paths::g_cacheLocation + Paths::Textures::g_metadataLocation + fwp._path + "/";
    const Str64 cacheName = (fwp._fileName + ".cache");

    ByteBuffer metadataCache = {};
    if (metadataCache.loadFromFile(cachePath.c_str(), cacheName.c_str())) {
        metadataCache >> _hasTransparency;
        metadataCache >> _hasTranslucency;
    } else {
        STUBBED("ToDo: Add support for 16bit and HDR image alpha! -Ionut");
        if (fileData.alpha()) {
            const auto findAlpha = [this, &fileData, height, layer](const Task* parent, U32 start, U32 end) {
                U8 tempA = 0u;
                for (U32 i = start; i < end; ++i) {
                    for (I32 j = 0; j < height; ++j) {
                        if (_hasTransparency && _hasTranslucency) {
                            return;
                        }
                        fileData.getAlpha(i, j, tempA, layer);
                        if (IS_IN_RANGE_INCLUSIVE(tempA, 0, 254)) {
                            _hasTransparency = true;
                            _hasTranslucency = tempA > 1;
                            if (_hasTranslucency) {
                                return;
                            }
                        }
                    }
                }
            };

            ParallelForDescriptor descriptor = {};
            descriptor._iterCount = width;
            descriptor._partitionSize = std::max(16u, to_U32(width / 10));
            descriptor._useCurrentThread = true;

            parallel_for(_context.context(), findAlpha, descriptor);

            metadataCache << _hasTransparency;
            metadataCache << _hasTranslucency;
            metadataCache.dumpToFile(cachePath.c_str(), cacheName.c_str());
        }
    }

    Console::printfn(Locale::get(_ID("TEXTURE_HAS_TRANSPARENCY_TRANSLUCENCY")),
                                    name.c_str(),
                                    _hasTransparency ? "yes" : "no",
                                    _hasTranslucency ? "yes" : "no");

    return true;
}

void Texture::setCurrentSampler(const SamplerDescriptor& descriptor) {
    // This can be called at any time
    _descriptor.samplerDescriptor(descriptor);
}

void Texture::setSampleCount(U8 newSampleCount) noexcept { 
    CLAMP(newSampleCount, to_U8(0u), _context.gpuState().maxMSAASampleCount());
    if (_descriptor._msaaSamples != newSampleCount) {
        _descriptor._msaaSamples = newSampleCount;
        resize({ NULL, 0 }, { width(), height() });
    }
}

void Texture::validateDescriptor() {
    
    // Select the proper colour space internal format
    if (_descriptor.baseFormat() == GFXImageFormat::RED ||
        _descriptor.baseFormat() == GFXImageFormat::RG ||
        _descriptor.baseFormat() == GFXImageFormat::DEPTH_COMPONENT)             
    {
        // We only support 8 bit per pixel - 3 & 4 channel textures
        assert(!_descriptor.srgb());
    }

    switch (_descriptor.baseFormat()) {
        case GFXImageFormat::COMPRESSED_RGB_DXT1: {
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT1: {
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT3: {
            _descriptor._compressed = true;
        } break;
        case GFXImageFormat::COMPRESSED_RGBA_DXT5: {
            _descriptor._compressed = true;
        } break;
    }

    if (!_descriptor._compressed) {
        if (_descriptor._samplerDescriptor.generateMipMaps()) {
            if (_descriptor._mipLevels.max <= 1) {
                _descriptor._mipLevels.max = computeMipCount(_width, _height) + 1;
                _descriptor._mipCount = _descriptor.mipLevels().max;
            }
        }
    }
}

U16 Texture::computeMipCount(U16 width, U16 height) noexcept {
    return to_U16(std::floorf(std::log2f(std::fmaxf(to_F32(width), to_F32(height)))));
}

};

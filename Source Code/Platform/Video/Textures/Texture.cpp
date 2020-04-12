#include "stdafx.h"

#include "Headers/Texture.h"

#include "Core/Headers/Console.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/ByteBuffer.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/File/Headers/FileManagement.h"

namespace Divide {

namespace TypeUtil {
    const char* WrapModeToString(TextureWrap wrapMode) noexcept {
        return Names::textureWrap[to_base(wrapMode)];
    }

    TextureWrap StringToWrapMode(const stringImpl& wrapMode) {
        for (U8 i = 0; i < to_U8(TextureWrap::COUNT); ++i) {
            if (strcmp(wrapMode.c_str(), Names::textureWrap[i]) == 0) {
                return static_cast<TextureWrap>(i);
            }
        }

        return TextureWrap::COUNT;
    }

    const char* TextureFilterToString(TextureFilter filter) noexcept {
        return Names::textureFilter[to_base(filter)];
    }

    TextureFilter StringToTextureFilter(const stringImpl& filter) {
        for (U8 i = 0; i < to_U8(TextureFilter::COUNT); ++i) {
            if (strcmp(filter.c_str(), Names::textureFilter[i]) == 0) {
                return static_cast<TextureFilter>(i);
            }
        }

        return TextureFilter::COUNT;
    }
};

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
      _data(0u, 0u, TextureType::COUNT),
      _descriptor(texDescriptor),
      _numLayers(texDescriptor._layerCount),
      _hasTransparency(false),
      _hasTranslucency(false),
      _flipped(isFlipped),
      _asyncLoad(asyncLoad)
{
    processTextureType();
    _width = _height = 0;
}

Texture::~Texture()
{
}

void Texture::processTextureType() noexcept {
    if (_descriptor.msaaSamples() == 0) {
        if (_descriptor.type() == TextureType::TEXTURE_2D_MS) {
            _descriptor.type(TextureType::TEXTURE_2D);
        }
        if (_descriptor.type() == TextureType::TEXTURE_2D_ARRAY_MS) {
            _descriptor.type(TextureType::TEXTURE_2D_ARRAY);
        }
    }
    else {
        if (_descriptor.type() == TextureType::TEXTURE_2D) {
            _descriptor.type(TextureType::TEXTURE_2D_MS);
        }
        if (_descriptor.type() == TextureType::TEXTURE_2D_ARRAY) {
            _descriptor.type(TextureType::TEXTURE_2D_ARRAY_MS);
        }
    }
    _data._textureType = _descriptor.type();
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

    TextureLoadInfo info = {};

    // Each texture face/layer must be in a comma separated list
    stringstreamImpl textureLocationList(assetLocation());
    stringstreamImpl textureFileList(assetName().c_str());

    bool loadFromFile = false;

    _descriptor._sourceFileList.reserve(6);

    hashMap<U64, ImageTools::ImageData> dataStorage;

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
            if (!loadFile(info, currentTextureFullPath, dataStorage[_ID(currentTextureFullPath.c_str())])) {
                // Invalid texture files are not handled yet, so stop loading
                continue;
            }

            info._layerIndex++;
            if (_data._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
                if (info._layerIndex == 6) {
                    info._layerIndex = 0;
                    info._cubeMapCount++;
                }
            }

            loadedFromFile = true;
        }
    }
    _descriptor._sourceFileList.shrink_to_fit();

    if (loadFromFile) {
        if (_data._textureType == TextureType::TEXTURE_CUBE_MAP ||
            _data._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._layerIndex != 6) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_CUBMAP_INIT_COUNT")),
                    resourceName().c_str());
                return;
            }
        }

        if (_data._textureType == TextureType::TEXTURE_2D_ARRAY ||
            _data._textureType == TextureType::TEXTURE_2D_ARRAY_MS) {
            if (info._layerIndex != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    resourceName().c_str());
                return;
            }
        }

        if (_data._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
            if (info._cubeMapCount != _numLayers) {
                Console::errorfn(
                    Locale::get(_ID("ERROR_TEXTURE_LOADER_ARRAY_INIT_COUNT")),
                    resourceName().c_str());
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
        fileData.set16Bit(_descriptor.dataType() == GFXDataFormat::FLOAT_16 ||
            _descriptor.dataType() == GFXDataFormat::SIGNED_SHORT ||
            _descriptor.dataType() == GFXDataFormat::UNSIGNED_SHORT);

        // Save file contents in  the "img" object
        ImageTools::ImageDataInterface::CreateImageData(name, _width, _height, _descriptor.srgb(), fileData);

        bufferPtr data = fileData.data();
        // Validate data
        if (data == nullptr) {
            if (info._layerIndex > 0) {
                Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LAYER_LOAD")), name.c_str());
                return false;
            }
            Console::errorfn(Locale::get(_ID("ERROR_TEXTURE_LOAD")), name.c_str());
            // Missing texture fallback.
            fileData.flip(false);
            // missing_texture.jpg must be something that really stands out
            ImageTools::ImageDataInterface::CreateImageData(((Paths::g_assetsLocation + Paths::g_texturesLocation) + s_missingTextureFileName).c_str(), _width, _height, _descriptor.srgb(), fileData);

        }

        // Extract width, height and bitdepth
        const U16 width = fileData.dimensions().width;
        const U16 height = fileData.dimensions().height;
        // If we have an alpha channel, we must check for translucency/transparency

        FileWithPath fwp = splitPathToNameAndLocation(name.c_str());
        Util::ReplaceStringInPlace(fwp._path, { "//","\\" }, "/");
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
                const auto findAlpha = [this, &fileData, height](const Task* parent, U32 start, U32 end) {
                    U8 tempA;
                    for (U32 i = start; i < end; ++i) {
                        for (I32 j = 0; j < height; ++j) {
                            if (_hasTransparency && _hasTranslucency) {
                                if (parent && parent->_parent) {
                                    Stop(*parent->_parent);
                                }
                                return;
                            }
                            fileData.getAlpha(i, j, tempA);
                            if (IS_IN_RANGE_INCLUSIVE(tempA, 0, 254)) {
                                _hasTransparency = true;
                                _hasTranslucency = tempA > 1;
                                if (_hasTranslucency) {
                                    if (parent && parent->_parent) {
                                        Stop(*parent->_parent);
                                    }
                                    return;
                                }
                            }
                        }
                        if (parent && ((parent->_parent && StopRequested(*parent->_parent)) || StopRequested(*parent))) {
                            break;
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
    }

    // Create a new Rendering API-dependent texture object
    if (info._layerIndex == 0) {
        _descriptor._type = _data._textureType;
        _descriptor._mipLevels.max = to_U16(fileData.mipCount());
        _descriptor._mipCount = _descriptor.mipLevels().max;
        _descriptor._compressed = fileData.compressed();
        _descriptor.baseFormat(fileData.format());
        _descriptor.dataType(fileData.dataType());
    } else {
        DIVIDE_ASSERT(_descriptor._type == _data._textureType, "Texture::loadFile error: Texture Layer with different type detected!");
    }
    // Uploading to the GPU dependents on the rendering API
    loadData(info, fileData.imageLayers());
    
    // We will always return true because we load the "missing_texture.jpg" in case of errors
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
        resize(NULL, { width(), height() });
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

    _data._textureType = _descriptor.type();
}

U16 Texture::computeMipCount(U16 width, U16 height) noexcept {
    return to_U16(std::floorf(std::log2f(std::fmaxf(to_F32(width), to_F32(height)))));
}

namespace XMLParser {
    void saveToXML(const SamplerDescriptor& sampler, const stringImpl& entryName, boost::property_tree::ptree& pt) {
        pt.put(entryName + ".Map.<xmlattr>.U", TypeUtil::WrapModeToString(sampler.wrapU()));
        pt.put(entryName + ".Map.<xmlattr>.V", TypeUtil::WrapModeToString(sampler.wrapV()));
        pt.put(entryName + ".Map.<xmlattr>.W", TypeUtil::WrapModeToString(sampler.wrapW()));
        pt.put(entryName + ".Filter.<xmlattr>.min", TypeUtil::TextureFilterToString(sampler.minFilter()));
        pt.put(entryName + ".Filter.<xmlattr>.mag", TypeUtil::TextureFilterToString(sampler.magFilter()));
        pt.put(entryName + ".anisotropy", to_U32(sampler.anisotropyLevel()));
    }

    void loadFromXML(SamplerDescriptor& sampler, const stringImpl& entryName, const boost::property_tree::ptree& pt) {
        sampler.wrapU(TypeUtil::StringToWrapMode(pt.get<stringImpl>(entryName + ".Map.<xmlattr>.U", TypeUtil::WrapModeToString(TextureWrap::REPEAT))));
        sampler.wrapV(TypeUtil::StringToWrapMode(pt.get<stringImpl>(entryName + ".Map.<xmlattr>.V", TypeUtil::WrapModeToString(TextureWrap::REPEAT))));
        sampler.wrapW(TypeUtil::StringToWrapMode(pt.get<stringImpl>(entryName + ".Map.<xmlattr>.W", TypeUtil::WrapModeToString(TextureWrap::REPEAT))));
        sampler.minFilter(TypeUtil::StringToTextureFilter(pt.get<stringImpl>(entryName + ".Filter.<xmlattr>.min", TypeUtil::TextureFilterToString(TextureFilter::LINEAR))));
        sampler.magFilter(TypeUtil::StringToTextureFilter(pt.get<stringImpl>(entryName + ".Filter.<xmlattr>.mag", TypeUtil::TextureFilterToString(TextureFilter::LINEAR))));
        sampler.anisotropyLevel(to_U8(pt.get(entryName + ".anisotropy", 0U)));
    }
};
};

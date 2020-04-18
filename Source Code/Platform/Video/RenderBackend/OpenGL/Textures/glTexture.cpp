#include "stdafx.h"

#include "config.h"

#include "Platform/Headers/PlatformRuntime.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glResources.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/glLockManager.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/RenderBackend/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(GFXDevice& context,
                     size_t descriptorHash,
                     const Str128& name,
                     const stringImpl& assetNames,
                     const stringImpl& assetLocations,
                     bool isFlipped,
                     bool asyncLoad,
                     const TextureDescriptor& texDescriptor)

    : Texture(context, descriptorHash, name, assetNames, assetLocations, isFlipped, asyncLoad, texDescriptor),
      glObject(glObjectType::TYPE_TEXTURE, context),
     _lockManager(MemoryManager_NEW glLockManager()),
     _loadingData(_data)
{
    _allocatedStorage = false;

    processTextureType();
    _loadingData._textureType = _descriptor.type();

    _type = GLUtil::glTextureTypeTable[to_U32(_descriptor.type())];

    U32 tempHandle = 0;
    if (GL_API::s_texturePool.typeSupported(_type)) {
        tempHandle = GL_API::s_texturePool.allocate(_type);
    } else {
        glCreateTextures(_type, 1, &tempHandle);
    }
    _loadingData._textureHandle = tempHandle;

    assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");
    _loadingData._samplerHandle = GL_API::getOrCreateSamplerObject(_descriptor.samplerDescriptor());

    if_constexpr(Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_TEXTURE, tempHandle, -1, name.c_str());
    }

    // Loading from file usually involves data that doesn't change, so call this here.
    addStateCallback(ResourceState::RES_LOADED, [this](CachedResource* res) {
        if (automaticMipMapGeneration() && _descriptor.samplerDescriptor().generateMipMaps()) {
            GL_API::queueComputeMipMap(_loadingData._textureHandle);
        }
        _data = _loadingData;
    });
}

glTexture::~glTexture()
{
    unload();
    MemoryManager::DELETE(_lockManager);
}

void glTexture::processTextureType() noexcept {
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
}

void glTexture::validateDescriptor() {
    Texture::validateDescriptor();
    _loadingData._textureType = _descriptor.type();
}

bool glTexture::unload() noexcept {
    assert(_data._textureType != TextureType::COUNT);

    U32 textureID = _data._textureHandle;
    if (textureID > 0) {
        if (_lockManager) {
            _lockManager->Wait(false);
        }
        GL_API::dequeueComputeMipMap(_data._textureHandle);
        if (GL_API::s_texturePool.typeSupported(_type)) {
            GL_API::s_texturePool.deallocate(textureID, _type);
        } else {
            Divide::GL_API::deleteTextures(1, &textureID, _descriptor.type());
        }
        _data._textureHandle = 0u;
    }

    return true;
}

void glTexture::threadedLoad() {

    Texture::threadedLoad();
    _lockManager->Lock(!Runtime::isMainThread());
    CachedResource::load();
}

void glTexture::setMipMapRange(U16 base, U16 max) noexcept {
    if (_descriptor.mipLevels() == vec2<U16>(base, max)) {
        return;
    }

    setMipRangeInternal(base, max);
    Texture::setMipMapRange(base, max);
}

void glTexture::setMipRangeInternal(U16 base, U16 max) {
    glTextureParameteri(_loadingData._textureHandle, GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_loadingData._textureHandle, GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const bufferPtr ptr, const vec2<U16>& dimensions) {
    const GLenum oldTexType = _type;
    const TextureType oldTexTypeDescriptor = _descriptor.type();
    _loadingData = _data;
    _data = {};

    processTextureType();
    _loadingData._textureType = _descriptor.type();
    _type = GLUtil::glTextureTypeTable[to_U32(_descriptor.type())];

    if (_loadingData._textureHandle > 0 && _allocatedStorage) {
        // Immutable storage requires us to create a new texture object 
        U32 tempHandle = 0;
        if (GL_API::s_texturePool.typeSupported(_type)) {
            tempHandle = GL_API::s_texturePool.allocate(_type);
        } else {
            glCreateTextures(_type, 1, &tempHandle);
        }
        assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");

        U32 handle = _loadingData._textureHandle;
        if (GL_API::s_texturePool.typeSupported(oldTexType)) {
            GL_API::s_texturePool.deallocate(handle, oldTexType);
        } else {
            Divide::GL_API::deleteTextures(1, &handle, oldTexTypeDescriptor);
        }
        _loadingData._textureHandle = tempHandle;
    }

    const vec2<U16> mipLevels(0, _descriptor.samplerDescriptor().generateMipMaps()
                                    ? 1 + Texture::computeMipCount(_width, _height)
                                    : 1);

    _allocatedStorage = false;
    // We may have limited the number of mips
    _descriptor.mipLevels({ mipLevels.x, std::min(_descriptor.mipLevels().y, mipLevels.y)});
    _descriptor.mipCount(mipLevels.y);

    const TextureLoadInfo info = {};
    loadData(info, ptr, dimensions);

    if (automaticMipMapGeneration() && _descriptor.samplerDescriptor().generateMipMaps()) {
        GL_API::queueComputeMipMap(_loadingData._textureHandle);
    }
    _data = _loadingData;
}

void glTexture::reserveStorage() {
    assert(
        !(_loadingData._textureType == TextureType::TEXTURE_CUBE_MAP && _width != _height) &&
        "glTexture::reserverStorage error: width and height for cube map texture do not match!");

    const GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    const GLuint handle = _loadingData._textureHandle;
    const GLuint msaaSamples = static_cast<GLuint>(_descriptor.msaaSamples());
    const GLushort mipMaxLevel = _descriptor.mipLevels().max;

    switch (_loadingData._textureType) {
        case TextureType::TEXTURE_1D: {
            glTextureStorage1D(
                handle,
                mipMaxLevel,
                glInternalFormat,
                _width);

        } break;
        case TextureType::TEXTURE_2D: {
            glTextureStorage2D(
                handle,
                mipMaxLevel,
                glInternalFormat,
                _width,
                _height);
        } break;
        case TextureType::TEXTURE_2D_MS: {
            glTextureStorage2DMultisample(
                handle,
                msaaSamples,
                glInternalFormat,
                _width,
                _height,
                GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY_MS: {
            glTextureStorage3DMultisample(
                handle,
                msaaSamples,
                glInternalFormat,
                _width,
                _height,
                _numLayers,
                GL_TRUE);
        } break;
        case TextureType::TEXTURE_3D:
        case TextureType::TEXTURE_2D_ARRAY:
        case TextureType::TEXTURE_CUBE_MAP:
        case TextureType::TEXTURE_CUBE_ARRAY: {
            U32 numFaces = 1;
            if (_loadingData._textureType == TextureType::TEXTURE_CUBE_MAP ||
                _loadingData._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
                numFaces = 6;
            }
            glTextureStorage3D(
                handle,
                mipMaxLevel,
                glInternalFormat,
                _width,
                _height,
                _numLayers * numFaces);
        } break;
        default:
            return;
    };
}

void glTexture::loadData(const TextureLoadInfo& info,
                         const bufferPtr data,
                         const vec2<U16>& dimensions) {
    // Create a new Rendering API-dependent texture object
    if (info._layerIndex == 0) {
        _descriptor.type( _loadingData._textureType);
    } else {
        DIVIDE_ASSERT(_descriptor.type() == _loadingData._textureType, "Texture::loadFile error: Texture Layer with different type detected!");
    }

    // This should never be called for compressed textures                            
    assert(!_descriptor.compressed());
    if (info._layerIndex == 0) {
        _width = dimensions.width;
        _height = dimensions.height;
 
        validateDescriptor();
        setMipRangeInternal(_descriptor.mipLevels().min, _descriptor.mipLevels().max);

        assert(_width > 0 && _height > 0);
    } else {
        assert(
            _width == dimensions.width &&
            _height == dimensions.height &&
            "glTexture error: Invalid dimensions for texture array layer");
    }

    bool expected = false;
    if (_allocatedStorage.compare_exchange_strong(expected, true)) {
        reserveStorage();
    }

    assert(_allocatedStorage);

    loadDataUncompressed(info, data);

    assert(_width > 0 && _height > 0 &&
        "glTexture error: Invalid texture dimensions!");

    if (getState() == ResourceState::RES_LOADED) {
        _data = _loadingData;
    }
}

void glTexture::loadData(const TextureLoadInfo& info,
                         const vectorEASTL<ImageTools::ImageLayer>& imageLayers) {
    if (info._layerIndex == 0) {
        _width = imageLayers[0]._dimensions.width;
        _height = imageLayers[0]._dimensions.height;

        assert(_width > 0 && _height > 0);

        validateDescriptor();

        //UniqueLock<Mutex> lock(GLUtil::_driverLock);
        setMipRangeInternal(_descriptor.mipLevels().min, _descriptor.mipLevels().max);
    } else {
        assert(
            _width == imageLayers[0]._dimensions.width && 
            _height == imageLayers[0]._dimensions.height &&
            "glTexture error: Invalid dimensions for texture array layer");
    }

    
    bool expected = false;
    if (_allocatedStorage.compare_exchange_strong(expected, true)) {
        //UniqueLock<Mutex> lock(GLUtil::_driverLock);
        reserveStorage();
    }
    assert(_allocatedStorage);

    if (_descriptor.compressed()) {
        //UniqueLock<Mutex> lock(GLUtil::_driverLock);
        loadDataCompressed(info, imageLayers);
    } else {
        //UniqueLock<Mutex> lock(GLUtil::_driverLock);
        loadDataUncompressed(info, imageLayers[0].data());
    }

    assert(_width > 0 && _height > 0 && "glTexture error: Invalid texture dimensions!");
    if (getState() == ResourceState::RES_LOADED) {
        _data = _loadingData;
    }
}

void glTexture::loadDataCompressed(const TextureLoadInfo& info,
                                   const vectorEASTL<ImageTools::ImageLayer>& imageLayers) {

    _descriptor.autoMipMaps(false);
    const GLenum glFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    const GLint numMips = static_cast<GLint>(imageLayers.size());

    GL_API::getStateTracker().setPixelPackUnpackAlignment();
    for (GLint i = 0; i < numMips; ++i) {
        const ImageTools::ImageLayer& layer = imageLayers[i];
        switch (_loadingData._textureType) {
            case TextureType::TEXTURE_1D: {
                glCompressedTextureSubImage1D(
                    _loadingData._textureHandle,
                    i,
                    0,
                    layer._dimensions.width,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer.data());
            } break;
            case TextureType::TEXTURE_2D: {
                glCompressedTextureSubImage2D(
                    _loadingData._textureHandle,
                    i,
                    0,
                    0,
                    layer._dimensions.width,
                    layer._dimensions.height,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer.data());
            } break;

            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glCompressedTextureSubImage3D(
                    _loadingData._textureHandle,
                    i,
                    0,
                    0,
                    (info._cubeMapCount * 6) + info._layerIndex,
                    layer._dimensions.width,
                    layer._dimensions.height,
                    layer._dimensions.depth,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer.data());
            } break;
            default:
                DIVIDE_UNEXPECTED_CALL("Unsupported texture format!");
                break;
        }
    };

    if (!Runtime::isMainThread()) {
        glFlush();
    }
}

void glTexture::loadDataUncompressed(const TextureLoadInfo& info, bufferPtr data) {
    if (data) {
        const GLenum format = GLUtil::glImageFormatTable[to_U32(_descriptor.baseFormat())];
        const GLenum type = GLUtil::glDataFormat[to_U32(_descriptor.dataType())];
        const GLuint handle = _loadingData._textureHandle;

        GL_API::getStateTracker().setPixelPackUnpackAlignment();
        switch (_loadingData._textureType) {
            case TextureType::TEXTURE_1D: {
                glTextureSubImage1D(handle, 0, 0, _width, format, type, data);
            } break;
            case TextureType::TEXTURE_2D:
            case TextureType::TEXTURE_2D_MS: {
                glTextureSubImage2D(handle, 0, 0, 0, _width, _height, format, type, data);
            } break;

            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glTextureSubImage3D(handle, 0, 0, 0, (info._cubeMapCount * 6) + info._layerIndex, _width, _height, 1, format, type, data);
            } break;
        }
    }
}


void glTexture::setCurrentSampler(const SamplerDescriptor& descriptor) {
    Texture::setCurrentSampler(descriptor);
    _data._samplerHandle = GL_API::getOrCreateSamplerObject(descriptor);
    _loadingData._samplerHandle = _data._samplerHandle;
}

void glTexture::bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) {
    assert(_data._textureType != TextureType::COUNT);

    const GLenum access = read ? (write ? GL_READ_WRITE : GL_READ_ONLY)
                               : (write ? GL_WRITE_ONLY : GL_NONE);

    const GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    GL_API::getStateTracker().bindTextureImage(slot, _descriptor.type(), _data._textureHandle, level, layered, layer, access, glInternalFormat);
}



/*static*/ void glTexture::copy(const TextureData& source, const TextureData& destination, const CopyTexParams& params) {
    OPTICK_EVENT();
    assert(source._textureType != TextureType::COUNT && destination._textureType != TextureType::COUNT);
    const TextureType srcType = source._textureType;
    const TextureType dstType = destination._textureType;
    if (srcType != TextureType::COUNT && dstType != TextureType::COUNT) {
        U32 numFaces = 1;
        if (srcType == TextureType::TEXTURE_CUBE_MAP || srcType == TextureType::TEXTURE_CUBE_ARRAY) {
            numFaces = 6;
        }

        glCopyImageSubData(
            //Source
            source._textureHandle,
            GLUtil::glTextureTypeTable[to_U32(srcType)],
            params._sourceMipLevel,
            params._sourceCoords.x,
            params._sourceCoords.y,
            params._sourceCoords.z,
            //Destination
            destination._textureHandle,
            GLUtil::glTextureTypeTable[to_U32(dstType)],
            params._targetMipLevel,
            params._targetCoords.x,
            params._targetCoords.y,
            params._targetCoords.z,
            //Source Dim
            params._dimensions.x,
            params._dimensions.y,
            params._dimensions.z * numFaces);
    }
}

};

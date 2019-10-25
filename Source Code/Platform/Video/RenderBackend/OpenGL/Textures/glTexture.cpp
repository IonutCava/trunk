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
                     const Str64& name,
                     const Str64& resourceName,
                     const stringImpl& resourceLocation,
                     bool isFlipped,
                     bool asyncLoad,
                     const TextureDescriptor& texDescriptor)

    : Texture(context, descriptorHash, name, resourceName, resourceLocation, isFlipped, asyncLoad, texDescriptor),
      glObject(glObjectType::TYPE_TEXTURE, context),
     _lockManager(MemoryManager_NEW glLockManager())
{
    _allocatedStorage = false;

    _type = GLUtil::glTextureTypeTable[to_U32(_descriptor.type())];

    U32 tempHandle = 0;
    if (GL_API::s_texturePool.typeSupported(_type)) {
        tempHandle = GL_API::s_texturePool.allocate(_type);
    } else {
        glCreateTextures(_type, 1, &tempHandle);
    }
    setTextureHandle(tempHandle);

    assert(data().textureHandle() != 0 && "glTexture error: failed to generate new texture handle!");
    setSamplerHandle(GL_API::getOrCreateSamplerObject(_descriptor.samplerDescriptor()));

    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_TEXTURE, _data.textureHandle(), -1, name.c_str());
    }
}

glTexture::~glTexture()
{
    unload();
    MemoryManager::DELETE(_lockManager);
}

bool glTexture::unload() noexcept {
    U32 textureID = _data.textureHandle();
    if (textureID > 0) {
        if (_lockManager) {
            _lockManager->Wait(false);
        }
        GL_API::dequeueComputeMipMap(_data.textureHandle());
        if (GL_API::s_texturePool.typeSupported(_type)) {
            GL_API::s_texturePool.deallocate(textureID, _type);
        } else {
            Divide::GL_API::deleteTextures(1, &textureID, _descriptor.type());
        }
        setTextureHandle(0u);
    }

    return true;
}

void glTexture::threadedLoad() {

    Texture::threadedLoad();

    // Loading from file usually involves data that doesn't change, so call this here.
    if (automaticMipMapGeneration() && _descriptor.samplerDescriptor().generateMipMaps()) {
        GL_API::queueComputeMipMap(_data.textureHandle());
    }
    CachedResource::load();

    _lockManager->Lock(!Runtime::isMainThread());
}

void glTexture::setMipMapRange(U16 base, U16 max) {
    if (_descriptor.mipLevels() == vec2<U16>(base, max)) {
        return;
    }

    setMipRangeInternal(base, max);
    Texture::setMipMapRange(base, max);
}

void glTexture::setMipRangeInternal(U16 base, U16 max) {
    glTextureParameteri(_data.textureHandle(), GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_data.textureHandle(), GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const bufferPtr ptr,
                       const vec2<U16>& dimensions) {

    if (_data.textureHandle() > 0 && _allocatedStorage) {
        // Immutable storage requires us to create a new texture object 
        U32 tempHandle = 0;
        if (GL_API::s_texturePool.typeSupported(_type)) {
            tempHandle = GL_API::s_texturePool.allocate(_type);
        } else {
            glCreateTextures(_type, 1, &tempHandle);
        }
        assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");

        U32 handle = _data.textureHandle();
        if (GL_API::s_texturePool.typeSupported(_type)) {
            GL_API::s_texturePool.deallocate(handle, _type);
        } else {
            Divide::GL_API::deleteTextures(1, &handle, _descriptor.type());
        }
        setTextureHandle(tempHandle);
    }

    vec2<U16> mipLevels(0, _descriptor.samplerDescriptor().generateMipMaps()
                                ? 1 + Texture::computeMipCount(_width, _height)
                                : 1);

    _allocatedStorage = false;
    // We may have limited the number of mips
    _descriptor.mipLevels({ mipLevels.x,
                            std::min(_descriptor.mipLevels().y, mipLevels.y)});
    _descriptor.mipCount(mipLevels.y);

    TextureLoadInfo info;
    loadData(info, ptr, dimensions);

    if (automaticMipMapGeneration() && _descriptor.samplerDescriptor().generateMipMaps()) {
        GL_API::queueComputeMipMap(_data.textureHandle());
    }
}

void glTexture::reserveStorage() {
    assert(
        !(data().type() == TextureType::TEXTURE_CUBE_MAP && _width != _height) &&
        "glTexture::reserverStorage error: width and height for cube map texture do not match!");

    GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    GLuint handle = _data.textureHandle();
    GLuint msaaSamples = static_cast<GLuint>(_descriptor.msaaSamples());
    GLushort mipMaxLevel = _descriptor.mipLevels().max;

    switch (_data.type()) {
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
            if (_data.type() == TextureType::TEXTURE_CUBE_MAP ||
                _data.type() == TextureType::TEXTURE_CUBE_ARRAY) {
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
    // This should never be called for compressed textures                            
    assert(!_descriptor.compressed());
    if (info._layerIndex == 0) {
        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
        }

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

}

void glTexture::loadData(const TextureLoadInfo& info,
                         const vector<ImageTools::ImageLayer>& imageLayers) {
    if (info._layerIndex == 0) {
        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = imageLayers[0]._dimensions.width;
            _height = imageLayers[0]._dimensions.height;
        }

        assert(_width > 0 && _height > 0);

        validateDescriptor();

        //UniqueLock lock(GLUtil::_driverLock);
        setMipRangeInternal(_descriptor.mipLevels().min, _descriptor.mipLevels().max);
    } else {
        assert(
            _width == imageLayers[0]._dimensions.width && 
            _height == imageLayers[0]._dimensions.height &&
            "glTexture error: Invalid dimensions for texture array layer");
    }

    
    bool expected = false;
    if (_allocatedStorage.compare_exchange_strong(expected, true)) {
        //UniqueLock lock(GLUtil::_driverLock);
        reserveStorage();
    }
    assert(_allocatedStorage);

    if (_descriptor.compressed()) {
        //UniqueLock lock(GLUtil::_driverLock);
        loadDataCompressed(info, imageLayers);
    } else {
        bufferPtr texData = nullptr;
        if (!imageLayers[0]._dataf.empty()) {
            texData = (bufferPtr)imageLayers[0]._dataf.data();
        } else if (!imageLayers[0]._data16.empty()) {
            texData = (bufferPtr)imageLayers[0]._data16.data();
        } else {
            texData = (bufferPtr)imageLayers[0]._data.data();
        }
        //UniqueLock lock(GLUtil::_driverLock);
        loadDataUncompressed(info, texData);
    }

    assert(_width > 0 && _height > 0 && "glTexture error: Invalid texture dimensions!");
}

void glTexture::loadDataCompressed(const TextureLoadInfo& info,
                                   const vector<ImageTools::ImageLayer>& imageLayers) {

    _descriptor.autoMipMaps(false);
    GLenum glFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    GLint numMips = static_cast<GLint>(imageLayers.size());

    GL_API::getStateTracker().setPixelPackUnpackAlignment();
    for (GLint i = 0; i < numMips; ++i) {
        const ImageTools::ImageLayer& layer = imageLayers[i];
        switch (_data.type()) {
            case TextureType::TEXTURE_1D: {
                glCompressedTextureSubImage1D(
                    _data.textureHandle(),
                    i,
                    0,
                    layer._dimensions.width,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer._data.data());
            } break;
            case TextureType::TEXTURE_2D: {
                glCompressedTextureSubImage2D(
                    _data.textureHandle(),
                    i,
                    0,
                    0,
                    layer._dimensions.width,
                    layer._dimensions.height,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer._data.data());
            } break;

            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glCompressedTextureSubImage3D(
                    _data.textureHandle(),
                    i,
                    0,
                    0,
                    (info._cubeMapCount * 6) + info._layerIndex,
                    layer._dimensions.width,
                    layer._dimensions.height,
                    layer._dimensions.depth,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer._data.data());
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
        GLenum format = GLUtil::glImageFormatTable[to_U32(_descriptor.baseFormat())];
        GLenum type = GLUtil::glDataFormat[to_U32(_descriptor.dataType())];
        GLuint handle = _data.textureHandle();

        GL_API::getStateTracker().setPixelPackUnpackAlignment();
        switch (_data.type()) {
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
    setSamplerHandle(GL_API::getOrCreateSamplerObject(descriptor));
}

void glTexture::bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) {
    GLenum access = read ? (write ? GL_READ_WRITE : GL_READ_ONLY)
                            : (write ? GL_WRITE_ONLY : GL_NONE);

    GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor.srgb());
    GL_API::getStateTracker().bindTextureImage(slot, _descriptor.type(), _data.textureHandle(), level, layered, layer, access, glInternalFormat);
}



/*static*/ void glTexture::copy(const TextureData& source, const TextureData& destination, const CopyTexParams& params) {
    const TextureType srcType = source.type();
    const TextureType dstType = destination.type();
    if (srcType != TextureType::COUNT && dstType != TextureType::COUNT) {
        U32 numFaces = 1;
        if (srcType == TextureType::TEXTURE_CUBE_MAP || srcType == TextureType::TEXTURE_CUBE_ARRAY) {
            numFaces = 6;
        }

        glCopyImageSubData(
            //Source
            source.textureHandle(),
            GLUtil::glTextureTypeTable[to_U32(srcType)],
            params._sourceMipLevel,
            params._sourceCoords.x,
            params._sourceCoords.y,
            params._sourceCoords.z,
            //Destination
            destination.textureHandle(),
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

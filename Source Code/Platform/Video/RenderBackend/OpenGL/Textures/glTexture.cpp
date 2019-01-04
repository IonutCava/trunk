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
                     const stringImpl& name,
                     const stringImpl& resourceName,
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
    glCreateTextures(_type, 1, &tempHandle);

    assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");
    _textureData.setHandle(tempHandle);
    _textureData._samplerHandle = GL_API::getOrCreateSamplerObject(_descriptor._samplerDescriptor);

    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_TEXTURE, tempHandle, -1, name.c_str());
    }
}

glTexture::~glTexture()
{
    unload();

    UniqueLock lock(_lockManagerMutex);
    MemoryManager::DELETE(_lockManager);
}

bool glTexture::unload() {
    U32 textureID = _textureData.getHandle();
    if (textureID > 0) {
        UniqueLock lock(_lockManagerMutex);
        if (_lockManager) {
            _lockManager->Wait(false);
        }
        Divide::GL_API::deleteTextures(1, &textureID, _descriptor.type());
        _textureData.setHandle(0U);
    }

    return _textureData.getHandle() == 0;
}

void glTexture::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {
    Texture::threadedLoad(onLoadCallback);

    // Loading from file usually involves data that doesn't change, so call this here.
    if (automaticMipMapGeneration() && _descriptor.getSampler().generateMipMaps()) {
        GL_API::queueComputeMipMap(_context.context(), _textureData.getHandle(), _asyncLoad);
    }

    {
        UniqueLock lock(_lockManagerMutex);
        _lockManager->Lock(!Runtime::isMainThread());
    }
    CachedResource::load(onLoadCallback);
}

void glTexture::setMipMapRange(U16 base, U16 max) {
    if (_descriptor._mipLevels == vec2<U16>(base, max)) {
        return;
    }

    setMipRangeInternal(base, max);
    Texture::setMipMapRange(base, max);
}

void glTexture::setMipRangeInternal(U16 base, U16 max) {
    glTextureParameteri(_textureData.getHandle(), GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_textureData.getHandle(), GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const bufferPtr ptr,
                       const vec2<U16>& dimensions) {

    U32 textureID = _textureData.getHandle();
    if (textureID > 0 && _allocatedStorage) {
        // Immutable storage requires us to create a new texture object 
        U32 tempHandle = 0;
        glCreateTextures(_type, 1, &tempHandle);
        assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");

        Divide::GL_API::deleteTextures(1, &textureID, _descriptor.type());
        textureID = tempHandle;
    }

    vec2<U16> mipLevels(0, _descriptor.getSampler().generateMipMaps()
                                ? 1 + Texture::computeMipCount(_width, _height)
                                : 1);

    _textureData.setHandle(textureID);
    _allocatedStorage = false;
    _descriptor._mipLevels.set(mipLevels);

    TextureLoadInfo info;
    loadData(info, ptr, dimensions);

    if (automaticMipMapGeneration() && _descriptor.getSampler().generateMipMaps()) {
        GL_API::queueComputeMipMap(_context.context(), _textureData.getHandle(), _asyncLoad);
    }
}

void glTexture::reserveStorage() {
    assert(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP && _width != _height) &&
        "glTexture::reserverStorage error: width and height for cube map texture do not match!");

    GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor._srgb);
    GLuint handle = _textureData.getHandle();
    GLuint msaaSamples = static_cast<GLuint>(_descriptor.msaaSamples());
    GLushort mipMaxLevel = _descriptor._mipLevels.max;

    switch (_textureData._textureType) {
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
            if (_textureData._textureType == TextureType::TEXTURE_CUBE_MAP ||
                _textureData._textureType == TextureType::TEXTURE_CUBE_ARRAY) {
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
    assert(!_descriptor._compressed);
    if (info._layerIndex == 0) {
        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
        }

        validateDescriptor();
        setMipRangeInternal(_descriptor._mipLevels.min, _descriptor._mipLevels.max);

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
        setMipRangeInternal(_descriptor._mipLevels.min, _descriptor._mipLevels.max);
    } else {
        assert(
            _width == imageLayers[0]._dimensions.width && 
            _height == imageLayers[0]._dimensions.height &&
            "glTexture error: Invalid dimensions for texture array layer");
    }

    bool expected = false;
    if (_allocatedStorage.compare_exchange_strong(expected, true)) {
        reserveStorage();
    }
    assert(_allocatedStorage);

    if (_descriptor._compressed) {
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
        loadDataUncompressed(info, texData);
    }

    assert(_width > 0 && _height > 0 && "glTexture error: Invalid texture dimensions!");
}

void glTexture::loadDataCompressed(const TextureLoadInfo& info,
                                   const vector<ImageTools::ImageLayer>& imageLayers) {

    GLenum glFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor._srgb);
    GLint numMips = static_cast<GLint>(imageLayers.size());

    GL_API::getStateTracker().setPixelPackUnpackAlignment();
    for (GLint i = 0; i < numMips; ++i) {
        const ImageTools::ImageLayer& layer = imageLayers[i];
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                glCompressedTextureSubImage1D(
                    _textureData.getHandle(),
                    i,
                    0,
                    layer._dimensions.width,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer._data.data());
            } break;
            case TextureType::TEXTURE_2D: {
                glCompressedTextureSubImage2D(
                    _textureData.getHandle(),
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
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glCompressedTextureSubImage3D(
                    _textureData.getHandle(),
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
        GLuint handle = _textureData.getHandle();

        GL_API::getStateTracker().setPixelPackUnpackAlignment();
        switch (_textureData._textureType) {
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

void glTexture::copy(const Texture_ptr& other) {
    {
        UniqueLock lock(_lockManagerMutex);
        if (_lockManager) {
            _lockManager->Wait(false);
        }
    }

    U32 numFaces = 1;
    TextureType type = other->getTextureType();
    if (type == TextureType::TEXTURE_CUBE_MAP || type == TextureType::TEXTURE_CUBE_ARRAY) {
        numFaces = 6;
    }

    glTexture* otherTex = std::dynamic_pointer_cast<glTexture>(other).get();

    GLuint srcHandle = otherTex->getData().getHandle();
    GLuint destHandle = _textureData.getHandle();

    glCopyImageSubData(//Source
                       srcHandle,
                       GLUtil::glTextureTypeTable[to_U32(type)],
                       0, //Level
                       0, //X
                       0, //Y
                       0, //Z
                       //Destination
                       destHandle,
                       _type,
                       0, //Level
                       0, //X
                       0, //Y
                       0, //Z
                       //Source Dim
                       otherTex->getWidth(),
                       otherTex->getHeight(),
                       otherTex->_numLayers * numFaces);
}

void glTexture::setCurrentSampler(const SamplerDescriptor& descriptor) {
    Texture::setCurrentSampler(descriptor);
    _textureData._samplerHandle = GL_API::getOrCreateSamplerObject(descriptor);
}

bool glTexture::resourceLoadComplete() {
    WAIT_FOR_CONDITION(getState() == ResourceState::RES_LOADED);

    {
        UniqueLock lock(_lockManagerMutex);
        if (_lockManager) {
            _lockManager->Wait(true);
            MemoryManager::DELETE(_lockManager);
        }
    }

    return Texture::resourceLoadComplete();
}

void glTexture::bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) {
    GLenum access = read ? (write ? GL_READ_WRITE : GL_READ_ONLY)
                            : (write ? GL_WRITE_ONLY : GL_NONE);

    GLenum glInternalFormat = GLUtil::internalFormat(_descriptor.baseFormat(), _descriptor.dataType(), _descriptor._srgb);
    GL_API::getStateTracker().bindTextureImage(slot, _descriptor.type(), _textureData.getHandle(), level, layered, layer, access, glInternalFormat);
}

};

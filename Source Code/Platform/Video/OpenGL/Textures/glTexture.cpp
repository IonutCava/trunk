#include "stdafx.h"

#include "config.h"

#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

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
      glObject(glObjectType::TYPE_TEXTURE),
     _lockManager(MemoryManager_NEW glLockManager())
{
    GL_API::getOrCreateSamplerObject(_descriptor.getSampler());

    _allocatedStorage = false;

    _type = GLUtil::glTextureTypeTable[to_U32(_descriptor.type())];

    U32 tempHandle = 0;
    glCreateTextures(_type, 1, &tempHandle);
    if (Config::ENABLE_GPU_VALIDATION) {
        glObjectLabel(GL_TEXTURE, tempHandle, -1, getName().c_str());
    }
    assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");
    _textureData.setHandleHigh(tempHandle);
}

glTexture::~glTexture()
{
    MemoryManager::DELETE(_lockManager);
}

bool glTexture::unload() {
    U32 textureID = _textureData.getHandleHigh();
    if (textureID > 0) {
        if (_lockManager) {
            _lockManager->Wait(false);
        }
        glDeleteTextures(1, &textureID);
        _textureData.setHandleHigh(0U);
    }

    return true;
}

void glTexture::threadedLoad(DELEGATE_CBK<void, CachedResource_wptr> onLoadCallback) {
    if (_asyncLoad) {
        GL_API::createOrValidateContextForCurrentThread();
    }

    Texture::threadedLoad(onLoadCallback);
    _lockManager->Lock();
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
    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const bufferPtr ptr,
                       const vec2<U16>& dimensions,
                       const vec2<U16>& mipLevels) {
    U32 textureID = _textureData.getHandleHigh();
    if (textureID > 0 && _allocatedStorage) {
        // Immutable storage requires us to create a new texture object 
        U32 tempHandle = 0;
        glCreateTextures(_type, 1, &tempHandle);
        assert(tempHandle != 0 && "glTexture error: failed to generate new texture handle!");

        glDeleteTextures(1, &textureID);
        textureID = tempHandle;
    }

    _textureData.setHandleHigh(textureID);
    _allocatedStorage = false;
    _descriptor._mipLevels.set(mipLevels);

    TextureLoadInfo info;
    loadData(info, ptr, dimensions);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _descriptor.automaticMipMapGeneration() && _descriptor.getSampler().generateMipMaps()) {
        glGenerateTextureMipmap(_textureData.getHandleHigh());
    }
    Texture::updateMipMaps();
}

void glTexture::reserveStorage() {
    assert(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP && _width != _height) &&
        "glTexture::reserverStorage error: width and height for cube map texture do not match!");

    GLenum glInternalFormat = GLUtil::glImageFormatTable[to_U32(_descriptor.internalFormat())];
    GLuint handle = _textureData.getHandleHigh();
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
    
    _allocatedStorage = true;
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

    if (!_allocatedStorage) {
        reserveStorage();
    }
    assert(_allocatedStorage);

    loadDataUncompressed(info, data);

    assert(_width > 0 && _height > 0 &&
        "glTexture error: Invalid texture dimensions!");

}

void glTexture::loadData(const TextureLoadInfo& info,
                         const vectorImpl<ImageTools::ImageLayer>& imageLayers) {
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

    if (!_allocatedStorage) {
        reserveStorage();
    }
    assert(_allocatedStorage);

    if (_descriptor._compressed) {
        loadDataCompressed(info, imageLayers);
    } else {
        loadDataUncompressed(info, (bufferPtr)imageLayers[0]._data.data());
    }

    // Loading from file usually involves data that doesn't change, so call this here.
    updateMipMaps();

    assert(_width > 0 && _height > 0 && "glTexture error: Invalid texture dimensions!");
}

void glTexture::loadDataCompressed(const TextureLoadInfo& info,
                                   const vectorImpl<ImageTools::ImageLayer>& imageLayers) {

    GLenum glFormat = GLUtil::glImageFormatTable[to_U32(_descriptor.baseFormat())];
    GLint numMips = static_cast<GLint>(imageLayers.size());

    GL_API::setPixelPackUnpackAlignment();
    for (GLint i = 0; i < numMips; ++i) {
        const ImageTools::ImageLayer& layer = imageLayers[i];
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                glCompressedTextureSubImage1D(
                    _textureData.getHandleHigh(),
                    i,
                    0,
                    layer._dimensions.width,
                    glFormat,
                    static_cast<GLsizei>(layer._size),
                    layer._data.data());
            } break;
            case TextureType::TEXTURE_2D: {
                glCompressedTextureSubImage2D(
                    _textureData.getHandleHigh(),
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
                    _textureData.getHandleHigh(),
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
    _mipMapsDirty = true;

    if (numMips == 1) {
        updateMipMaps();
    }
}

void glTexture::loadDataUncompressed(const TextureLoadInfo& info, bufferPtr data) {
    if (data) {
        GLenum format = GLUtil::glImageFormatTable[to_U32(_descriptor.baseFormat())];
        GLenum type = GLUtil::glDataFormat[to_U32(_descriptor.dataType())];
        GLuint handle = _textureData.getHandleHigh();

        GL_API::setPixelPackUnpackAlignment();
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                glTextureSubImage1D(handle, 0, 0, _width, format, type, data);
                _mipMapsDirty = true;
            } break;
            case TextureType::TEXTURE_2D:
            case TextureType::TEXTURE_2D_MS: {
                glTextureSubImage2D(handle, 0, 0, 0, _width, _height, format, type, data);
                _mipMapsDirty = true;
            } break;

            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glTextureSubImage3D(handle, 0, 0, 0, (info._cubeMapCount * 6) + info._layerIndex, _width, _height, 1, format, type, data);
                _mipMapsDirty = info._layerIndex == _numLayers - 1;
            } break;
        }
    }

    updateMipMaps();

    if (!Config::USE_GPU_THREADED_LOADING) {
        if (_context.getGPUVendor() == GPUVendor::NVIDIA) {
        } else if (_context.getGPUVendor() == GPUVendor::AMD) {
            STUBBED("WHYYYYY do we need to flush texture uploads in single-thread mode???? I should investigate this, but multi-threaded is the default so it's low prio -Ionut");
            glFlush();
        } else {
        }
    }
}

void glTexture::copy(const Texture_ptr& other) {
    if (_lockManager) {
        _lockManager->Wait(false);
    }

    U32 numFaces = 1;
    TextureType type = other->getTextureType();
    if (type == TextureType::TEXTURE_CUBE_MAP || type == TextureType::TEXTURE_CUBE_ARRAY) {
        numFaces = 6;
    }

    glTexture* otherTex = std::dynamic_pointer_cast<glTexture>(other).get();

    GLuint srcHandle = otherTex->getData().getHandleHigh();
    GLuint destHandle = _textureData.getHandleHigh();

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
    GL_API::getOrCreateSamplerObject(descriptor);
}

bool glTexture::flushTextureState() {
    if (_lockManager) {
        WAIT_FOR_CONDITION(getState() == ResourceState::RES_LOADED);
        _lockManager->Wait(true);
        MemoryManager::DELETE(_lockManager);
    }

    updateMipMaps();

    return true;
}

void glTexture::bind(U8 unit) {
    flushTextureState();

    GL_API::bindTexture(unit, _textureData.getHandleHigh(), _type, _textureData._samplerHash);
}

void glTexture::bindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write) {
    flushTextureState();

    GLenum access = read ? (write ? GL_READ_WRITE : GL_READ_ONLY)
                            : (write ? GL_WRITE_ONLY : GL_NONE);
    GL_API::bindTextureImage(slot, _textureData.getHandleHigh(), level, layered, layer, access, 
                                GLUtil::glImageFormatTable[to_U32(_descriptor.internalFormat())]);
}

};

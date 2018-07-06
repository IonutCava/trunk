#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Headers/GLWrapper.h"

#include "Core/Headers/ParamHandler.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(GFXDevice& context, TextureType type)
    : Texture(context, type),
    _lockManager(new glLockManager())
{
    _allocatedStorage = false;

    _type = GLUtil::glTextureTypeTable[to_uint(type)];

    U32 tempHandle = 0;
    glCreateTextures(_type, 1, &tempHandle);
    
    DIVIDE_ASSERT(tempHandle != 0,
                  "glTexture error: failed to generate new texture handle!");
    _textureData.setHandleHigh(tempHandle);
}

glTexture::~glTexture()
{
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

void glTexture::threadedLoad(const stringImpl& name) {
    updateSampler();
    Texture::load();
    if (_threadedLoading) {
        _lockManager->Lock();
    } else {
        _lockManager.reset(nullptr);
    }
    Resource::threadedLoad(name);
}

void glTexture::setMipMapRange(GLushort base, GLushort max) {
    if (_mipMinLevel == base && _mipMaxLevel == max) {
        return;
    }

    Texture::setMipMapRange(base, max);

    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const bufferPtr ptr,
                       const vec2<U16>& dimensions,
                       const vec2<U16>& mipLevels) {
    // Immutable storage requires us to create a new texture object 
    U32 tempHandle = 0;
    glCreateTextures(_type, 1, &tempHandle);
    
    DIVIDE_ASSERT(tempHandle != 0,
        "glTexture error: failed to generate new texture handle!");

    U32 textureID = _textureData.getHandleHigh();
    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
    }
    _textureData.setHandleHigh(tempHandle);
    _allocatedStorage = false;
    TextureLoadInfo info;
    loadData(info, _descriptor, ptr, dimensions, mipLevels);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _descriptor._samplerDescriptor.generateMipMaps()) {
        glGenerateTextureMipmap(_textureData.getHandleHigh());
    }
    _mipMapsDirty = false;
}

void glTexture::updateSampler() {
    if (_samplerDirty) {
        _textureData._samplerHash = 
            GL_API::getOrCreateSamplerObject(_descriptor._samplerDescriptor);
        _samplerDirty = false;
    }
}

bool glTexture::load() {
    _context.loadInContext(
        _threadedLoading ? CurrentContext::GFX_LOADING_CTX
                         : CurrentContext::GFX_RENDERING_CTX,
        [&]() {
            threadedLoad(_name);
        });

    return true;
}

void glTexture::reserveStorage(const TextureLoadInfo& info) {
    DIVIDE_ASSERT(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP &&
          _width != _height),
        "glTexture::reserverStorage error: width and height for "
        "cube map texture do not match!");

    ParamHandler& par = ParamHandler::getInstance();

    GLenum glInternalFormat = _descriptor._internalFormat == GFXImageFormat::DEPTH_COMPONENT
                            ? GL_DEPTH_COMPONENT32
                            : GLUtil::glImageFormatTable[to_uint(_descriptor._internalFormat)];
    switch (_textureData._textureType) {
        case TextureType::TEXTURE_1D: {
            glTextureStorage1D(
                _textureData.getHandleHigh(),
                _mipMaxLevel,
                glInternalFormat,
                _width);

        } break;
        //case TextureType::TEXTURE_CUBE_MAP:
        case TextureType::TEXTURE_2D: {
            glTextureStorage2D(
                _textureData.getHandleHigh(),
                _mipMaxLevel,
                glInternalFormat,
                _width,
                _height);
        } break;
        case TextureType::TEXTURE_2D_MS: {
            glTextureStorage2DMultisample(
                _textureData.getHandleHigh(), 
                par.getParam<I32>("rendering.MSAAsampless", 0),
                glInternalFormat,
                _width,
                _height,
                GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY_MS: {
            glTextureStorage3DMultisample(
                _textureData.getHandleHigh(),
                par.getParam<I32>("rendering.MSAAsampless", 0),
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
                _textureData.getHandleHigh(),
                _mipMaxLevel,
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
                         const TextureDescriptor& descriptor,
                         const bufferPtr ptr,
                         const vec2<GLushort>& dimensions,
                         const vec2<GLushort>& mipLevels) {
    _descriptor = descriptor;

    GLenum glFormat = GLUtil::glImageFormatTable[to_uint(_descriptor.baseFormat())];
    GLenum dataType = GLUtil::glDataFormat[to_uint(_descriptor.dataType())];

    if (info._layerIndex == 0) {
            
        setMipMapRange(mipLevels.x, mipLevels.y);

        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
            _power2Size = true;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
            assert(_width > 0 && _height > 0);
            GLdouble xPow2 = log((GLdouble)_width) / log(2.0);
            GLdouble yPow2 = log((GLdouble)_height) / log(2.0);

            GLushort ixPow2 = (GLushort)xPow2;
            GLushort iyPow2 = (GLushort)yPow2;

            if (xPow2 != (GLdouble)ixPow2) {
                ixPow2++;
            }
            if (yPow2 != (GLdouble)iyPow2) {
                iyPow2++;
            }
            _power2Size = (_width == 1 << ixPow2) && (_height == 1 << iyPow2);
        }
    } else {
        DIVIDE_ASSERT(
            _width == dimensions.width && _height == dimensions.height,
            "glTexture error: Invalid dimensions for texture array layer");
    }

    if (!_allocatedStorage) {
        reserveStorage(info);
    }

    assert(_allocatedStorage);

    bool generateMipMaps = false;
    if (ptr) {
        GL_API::setPixelPackUnpackAlignment();
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                glTextureSubImage1D(
                    _textureData.getHandleHigh(),
                    0,
                    0,
                    _width,
                    glFormat,
                    dataType,
                    ptr);
                generateMipMaps = true;
            } break;
            case TextureType::TEXTURE_2D:
            case TextureType::TEXTURE_2D_MS: {
                glTextureSubImage2D(
                    _textureData.getHandleHigh(),
                    0, 
                    0,
                    0,
                    _width,
                    _height,
                    glFormat,
                    dataType,
                    ptr);
                generateMipMaps = true;
            } break;

            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS:
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                glTextureSubImage3D(
                    _textureData.getHandleHigh(),
                    0,
                    0,
                    0,
                    (info._cubeMapCount * 6) + info._layerIndex,
                    _width,
                    _height,
                    1,
                    glFormat,
                    dataType,
                    ptr);
                generateMipMaps = info._layerIndex == _numLayers;
            } break;
        }
    }

    if (generateMipMaps) {
        glGenerateTextureMipmap(_textureData.getHandleHigh());
        _mipMapsDirty = false;
    }

    DIVIDE_ASSERT(_width > 0 && _height > 0,
                  "glTexture error: Invalid texture dimensions!");
}

bool glTexture::flushTextureState() {
    if (!_threadedLoadComplete) {
        return false;
    }
    if (_lockManager) {
        _lockManager->Wait(true);
        _lockManager.reset(nullptr);
    }
    updateSampler();
    updateMipMaps();
    return true;
}

void glTexture::Bind(U8 unit, bool flushStateOnRequest) {
    if ((flushStateOnRequest && flushTextureState()) || !flushStateOnRequest) {
        GL_API::bindTexture(unit, _textureData.getHandleHigh(), _type,
                            _textureData._samplerHash);
    }
}

void glTexture::BindLayer(U8 slot, U8 level, U8 layer, bool layered, bool read, bool write, bool flushStateOnRequest) {
    if ((flushStateOnRequest && flushTextureState()) || !flushStateOnRequest) {
        GLenum access = read ? (write ? GL_READ_WRITE : GL_READ_ONLY)
                             : (write ? GL_WRITE_ONLY : GL_NONE);
        GL_API::bindTextureImage(slot, _textureData.getHandleHigh(), level, layered, layer, access, 
                                 GLUtil::glImageFormatTable[to_uint(_descriptor._internalFormat)]);
    }
}

};

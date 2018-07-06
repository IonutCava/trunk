#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(TextureType type, bool flipped)
    : Texture(type, flipped),
    _lockManager(new glLockManager(true))
{
    _format = _internalFormat = GL_NONE;
    _allocatedStorage = false;
    _type = GLUtil::glTextureTypeTable[to_uint(type)];
    U32 tempHandle = 0;
    GLUtil::DSAWrapper::dsaCreateTextures(_type, 1, &tempHandle);
    DIVIDE_ASSERT(tempHandle != 0,
                  "glTexture error: failed to generate new texture handle!");
    _textureData.setHandleHigh(tempHandle);
    _mipMaxLevel = _mipMinLevel = 0;
}

glTexture::~glTexture()
{
}

bool glTexture::unload() {
    U32 textureID = _textureData.getHandleHigh();
    if (textureID > 0) {
        glDeleteTextures(1, &textureID);
        _textureData.setHandleHigh(0U);
    }

    return true;
}

void glTexture::threadedLoad(const stringImpl& name) {
    updateSampler();
    Texture::generateHWResource(name);
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

    _mipMinLevel = base;
    _mipMaxLevel = max;

    GLUtil::DSAWrapper::dsaTextureParameter(_textureData.getHandleHigh(), _type,
                                            GL_TEXTURE_BASE_LEVEL, base);
    GLUtil::DSAWrapper::dsaTextureParameter(_textureData.getHandleHigh(), _type,
                                            GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _samplerDescriptor.generateMipMaps()) {
        GLUtil::DSAWrapper::dsaGenerateTextureMipmap(
            _textureData.getHandleHigh(), _type);
    }
    _mipMapsDirty = false;
}

void glTexture::updateSampler() {
    if (_samplerDirty) {
        _textureData._samplerHash = 
            GL_API::getOrCreateSamplerObject(_samplerDescriptor);
        _samplerDirty = false;
    }
}

bool glTexture::generateHWResource(const stringImpl& name) {
    STUBBED("ToDo: Remove this hack so that loading textures in a separate thread is possible on AMD - Ionut")
    if (GFX_DEVICE.getGPUVendor() == GPUVendor::AMD) {
        _threadedLoading = false;
    }

    GFX_DEVICE.loadInContext(
        _threadedLoading ? CurrentContext::GFX_LOADING_CTX
                         : CurrentContext::GFX_RENDERING_CTX,
        DELEGATE_BIND(&glTexture::threadedLoad, this, name));

    return true;
}

void glTexture::reserveStorage() {
    DIVIDE_ASSERT(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP &&
          _width != _height),
        "glTexture::reserverStorage error: width and heigth for "
        "cube map texture do not match!");

    switch (_textureData._textureType) {
        case TextureType::TEXTURE_1D: {
            GLUtil::DSAWrapper::dsaTextureStorage(
                _textureData.getHandleHigh(), _type, _mipMaxLevel,
                _internalFormat, _width, -1, -1);

        } break;
        case TextureType::TEXTURE_2D:
        case TextureType::TEXTURE_CUBE_MAP: {
            GLUtil::DSAWrapper::dsaTextureStorage(
                _textureData.getHandleHigh(), _type, _mipMaxLevel,
                _internalFormat, _width, _height, -1);
        } break;
        case TextureType::TEXTURE_2D_MS: {
            GLUtil::DSAWrapper::dsaTextureStorageMultisample(
                _textureData.getHandleHigh(), _type,
                GFX_DEVICE.gpuState().MSAASamples(), _internalFormat, _width,
                _height, -1, GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY_MS: {
            GLUtil::DSAWrapper::dsaTextureStorageMultisample(
                _textureData.getHandleHigh(), _type,
                GFX_DEVICE.gpuState().MSAASamples(), _internalFormat, _width,
                _height, _numLayers, GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY:
        case TextureType::TEXTURE_CUBE_ARRAY:
        case TextureType::TEXTURE_3D: {
            // Use _imageLayers as depth for GL_TEXTURE_3D
            GLUtil::DSAWrapper::dsaTextureStorage(
                _textureData.getHandleHigh(), _type, _mipMaxLevel,
                _internalFormat, _width, _height, _numLayers);
        } break;
        default:
            return;
    };

    _allocatedStorage = true;
}

void glTexture::loadData(GLuint target,
                         const GLubyte* const ptr,
                         const vec2<GLushort>& dimensions,
                         const vec2<GLushort>& mipLevels,
                         GFXImageFormat format,
                         GFXImageFormat internalFormat) {
    bool isTextureLayer =
        (_textureData._textureType == TextureType::TEXTURE_2D_ARRAY &&
         target > 0);

    if (!isTextureLayer) {
        _format = GLUtil::glImageFormatTable[to_uint(format)];
        _internalFormat =
            internalFormat == GFXImageFormat::DEPTH_COMPONENT
                ? GL_DEPTH_COMPONENT32
                : GLUtil::glImageFormatTable[to_uint(internalFormat)];
        setMipMapRange(mipLevels.x, mipLevels.y);

        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
            assert(_width > 0 && _height > 0);
        }
    } else {
        DIVIDE_ASSERT(
            _width == dimensions.width && _height == dimensions.height,
            "glTexture error: Invalid dimensions for texture array layer");
    }

    if (!_allocatedStorage) {
        reserveStorage();
    }
    assert(_allocatedStorage);

    if (ptr) {
        assert(_bitDepth != 0);
        GLdouble xPow2 = log((GLdouble)_width)  / log(2.0);
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

        GL_API::setPixelPackUnpackAlignment();
        switch (_textureData._textureType) {
            case TextureType::TEXTURE_1D: {
                GLUtil::DSAWrapper::dsaTextureSubImage(
                    _textureData.getHandleHigh(), _type, 0, 0, 0, 0,
                    _width, -1, -1, _format, GL_UNSIGNED_BYTE, (bufferPtr)ptr);
            } break;
            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS: {
                GLUtil::DSAWrapper::dsaTextureSubImage(
                    _textureData.getHandleHigh(), _type, 0, 0, 0, 0,
                    _width, _height, 1, _format, GL_UNSIGNED_BYTE, (bufferPtr)ptr);
            } break;
            case TextureType::TEXTURE_CUBE_MAP:
            case TextureType::TEXTURE_CUBE_ARRAY: {
                GLUtil::DSAWrapper::dsaTextureSubImage(
                    _textureData.getHandleHigh(), _type, 0, 0, 0, target,
                    _width, _height, 1, _format, GL_UNSIGNED_BYTE, (bufferPtr)ptr);
            } break;
            default:
            case TextureType::TEXTURE_2D:
            case TextureType::TEXTURE_2D_MS: {
                GLUtil::DSAWrapper::dsaTextureSubImage(
                    _textureData.getHandleHigh(), _type, 0, 0, 0, 0,
                    _width, _height, -1, _format, GL_UNSIGNED_BYTE, (bufferPtr)ptr);
            } break;
        }
        _mipMapsDirty = true;
        updateMipMaps();
    }

    DIVIDE_ASSERT(_width > 0 && _height > 0,
                  "glTexture error: Invalid texture dimensions!");
}

void glTexture::Bind(GLubyte unit) {
    if (!_threadedLoadComplete) {
        return;
    }
    if (_lockManager) {
        _lockManager->Wait();
        _lockManager.reset(nullptr);
    }
    updateSampler();
    updateMipMaps();
    GL_API::bindTexture(unit, _textureData.getHandleHigh(), _type,
                        _textureData._samplerHash);
}
};
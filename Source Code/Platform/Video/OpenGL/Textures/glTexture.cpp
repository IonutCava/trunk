#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Platform/Video/OpenGL/Headers/glLockManager.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(TextureType type)
    : Texture(type),
    _lockManager(new glLockManager(true))
{
    _format = _internalFormat = GFXImageFormat::COUNT;
    _allocatedStorage = false;
    _type = GLUtil::glTextureTypeTable[to_uint(type)];
    U32 tempHandle = 0;
    glCreateTextures(_type, 1, &tempHandle);
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

    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_BASE_LEVEL, base);
    glTextureParameteri(_textureData.getHandleHigh(), GL_TEXTURE_MAX_LEVEL, max);
}

void glTexture::resize(const U8* const ptr,
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
    loadData(info, ptr, dimensions, mipLevels, _format, _internalFormat);
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _samplerDescriptor.generateMipMaps()) {
        glGenerateTextureMipmap(_textureData.getHandleHigh());
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

void glTexture::reserveStorage(TextureLoadInfo info) {
    DIVIDE_ASSERT(
        !(_textureData._textureType == TextureType::TEXTURE_CUBE_MAP &&
          _width != _height),
        "glTexture::reserverStorage error: width and heigth for "
        "cube map texture do not match!");

    GLenum glInternalFormat = _internalFormat == GFXImageFormat::DEPTH_COMPONENT
                            ? GL_DEPTH_COMPONENT32
                            : GLUtil::glImageFormatTable[to_uint(_internalFormat)];
    switch (_textureData._textureType) {
        case TextureType::TEXTURE_1D: {
            glTextureStorage1D(
                _textureData.getHandleHigh(),
                _mipMaxLevel,
                glInternalFormat,
                _width);

        } break;
        case TextureType::TEXTURE_CUBE_MAP:
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
                GFX_DEVICE.gpuState().MSAASamples(),
                glInternalFormat,
                _width,
                _height,
                GL_TRUE);
        } break;
        case TextureType::TEXTURE_2D_ARRAY_MS: {
            glTextureStorage3DMultisample(
                _textureData.getHandleHigh(),
                GFX_DEVICE.gpuState().MSAASamples(),
                glInternalFormat,
                _width,
                _height,
                _numLayers,
                GL_TRUE);
        } break;
        case TextureType::TEXTURE_3D:
        case TextureType::TEXTURE_2D_ARRAY: {
            glTextureStorage3D(
                _textureData.getHandleHigh(),
                _mipMaxLevel,
                glInternalFormat,
                _width,
                _height,
                _numLayers);
        } break;
        case TextureType::TEXTURE_CUBE_ARRAY: {
            glTextureStorage3D(
                _textureData.getHandleHigh(), 
                _mipMaxLevel,
                glInternalFormat,
                _width,
                _height,
                _numLayers * 6);
        } break;
        default:
            return;
    };

    _allocatedStorage = true;
}

void glTexture::loadData(TextureLoadInfo info,
                         const GLubyte* const ptr,
                         const vec2<GLushort>& dimensions,
                         const vec2<GLushort>& mipLevels,
                         GFXImageFormat format,
                         GFXImageFormat internalFormat) {
    _format = format;
    _internalFormat = internalFormat;
    GLenum glFormat = GLUtil::glImageFormatTable[to_uint(format)];

    if (info._layerIndex == 0) {
            
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
        reserveStorage(info);
    }

    assert(_allocatedStorage);

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
                    GL_UNSIGNED_BYTE,
                    (bufferPtr)ptr);
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
                    GL_UNSIGNED_BYTE,
                    (bufferPtr)ptr);
            } break;
            case TextureType::TEXTURE_3D:
            case TextureType::TEXTURE_2D_ARRAY:
            case TextureType::TEXTURE_2D_ARRAY_MS: {
                glTextureSubImage3D(
                    _textureData.getHandleHigh(),
                    0,
                    0,
                    0,
                    info._layerIndex,
                    _width,
                    _height,
                    1,
                    glFormat,
                    GL_UNSIGNED_BYTE,
                    (bufferPtr)ptr);
            } break;
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
                    GL_UNSIGNED_BYTE,
                    (bufferPtr)ptr);
            } break;
        }
    }
    
    _mipMapsDirty = true;
    updateMipMaps();

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

#include "Platform/Video/OpenGL/Headers/glResources.h"

#include "Headers/glTexture.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

glTexture::glTexture(TextureType type, bool flipped)
    : Texture(type, flipped)

{
    _format = _internalFormat = GL_NONE;
    _allocatedStorage = false;
    U32 tempHandle = 0;
    glGenTextures(1, &tempHandle);
    DIVIDE_ASSERT(tempHandle != 0,
                  "glTexture error: failed to generate new texture handle!");
    _textureData._textureHandle = tempHandle;
    _mipMaxLevel = _mipMinLevel = 0;
}

glTexture::~glTexture() {
}

bool glTexture::unload() {
    if (_textureData._textureHandle > 0) {
        U32 textureID = _textureData._textureHandle;
        glDeleteTextures(1, &textureID);
        _textureData._textureHandle = 0;
    }

    return true;
}

void glTexture::threadedLoad(const stringImpl& name) {
    updateSampler();
    Texture::generateHWResource(name);
    Resource::threadedLoad(name);
}

void glTexture::setMipMapRange(GLushort base, GLushort max) {
    if (!_textureData._samplerDescriptor.generateMipMaps()) {
        _mipMaxLevel = 1;
        return;
    }

    if (_mipMinLevel == base && _mipMaxLevel == max) {
        return;
    }

    _mipMinLevel = base;
    _mipMaxLevel = max;

#ifdef GL_VERSION_4_5
    glTextureParameterf(_textureData._textureHandle, GL_TEXTURE_BASE_LEVEL,
                        base);
    glTextureParameterf(_textureData._textureHandle, GL_TEXTURE_MAX_LEVEL, max);
#else
    GLenum type = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
        _textureData._textureType)];
    gl44ext::glTextureParameteriEXT(_textureData._textureHandle,
                                    GL_TEXTURE_BASE_LEVEL, type, base);
    gl44ext::glTextureParameteriEXT(_textureData._textureHandle,
                                    GL_TEXTURE_MAX_LEVEL, type, max);
#endif
}

void glTexture::updateMipMaps() {
    if (_mipMapsDirty && _textureData._samplerDescriptor.generateMipMaps()) {
        GLenum type = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
            _textureData._textureType)];
#ifdef GL_VERSION_4_5
        glGenerateTextureMipmap(_textureData._textureHandle, type);
#else
        gl44ext::glGenerateTextureMipmapEXT(_textureData._textureHandle, type);
#endif
        _mipMapsDirty = false;
    }
}

void glTexture::updateSampler() {
    if (_samplerDirty) {
        _samplerHash =
            GL_API::getOrCreateSamplerObject(_textureData._samplerDescriptor);
        _samplerDirty = false;
    }
}

bool glTexture::generateHWResource(const stringImpl& name) {
    GFX_DEVICE.loadInContext(
        _threadedLoading ? CurrentContext::GFX_LOADING_CONTEXT
                         : CurrentContext::GFX_RENDERING_CONTEXT,
        DELEGATE_BIND(&glTexture::threadedLoad, this, name));

    return true;
}

void glTexture::reserveStorage() {
    // generate empty texture data using each texture type's specific function
    GLenum type = GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
        _textureData._textureType)];
    switch (type) {
        case GL_TEXTURE_1D: {
#ifdef GL_VERSION_4_5
            glTextureStorage1D(_textureData._textureHandle, type, _mipMaxLevel,
                               _internalFormat, _width);
#else
            gl44ext::glTextureStorage1DEXT(_textureData._textureHandle, type,
                                           _mipMaxLevel, _internalFormat,
                                           _width);
#endif

        } break;
        case GL_TEXTURE_2D_MULTISAMPLE: {
#ifdef GL_VERSION_4_5
            glTextureStorage2DMultisample(_textureData._textureHandle, type,
                                          GFX_DEVICE.gpuState().MSAASamples(),
                                          _internalFormat, _width, _height,
                                          GL_TRUE);
#else
            gl44ext::glTextureStorage2DMultisampleEXT(
                _textureData._textureHandle, type,
                GFX_DEVICE.gpuState().MSAASamples(), _internalFormat, _width,
                _height, GL_TRUE);
#endif
        } break;
        case GL_TEXTURE_2D:
        case GL_TEXTURE_CUBE_MAP: {
#ifdef GL_VERSION_4_5
            glTextureStorage2D(_textureData._textureHandle, type, _mipMaxLevel,
                               _internalFormat, _width, _height);
#else
            gl44ext::glTextureStorage2DEXT(_textureData._textureHandle, type,
                                           _mipMaxLevel, _internalFormat,
                                           _width, _height);
#endif
        } break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY: {
#ifdef GL_VERSION_4_5
            glTextureStorage3DMultisample(_textureData._textureHandle, type,
                                          GFX_DEVICE.gpuState().MSAASamples(),
                                          _internalFormat, _width, _height,
                                          _numLayers, GL_TRUE);
#else
            gl44ext::glTextureStorage3DMultisampleEXT(
                _textureData._textureHandle, type,
                GFX_DEVICE.gpuState().MSAASamples(), _internalFormat, _width,
                _height, _numLayers, GL_TRUE);
#endif
        } break;
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_CUBE_MAP_ARRAY:
        case GL_TEXTURE_3D: {
// Use _imageLayers as depth for GL_TEXTURE_3D
#ifdef GL_VERSION_4_5
            glTextureStorage3D(_textureData._textureHandle, type, _mipMaxLevel,
                               _internalFormat, _width, _height, _numLayers);
#else
            gl44ext::glTextureStorage3DEXT(_textureData._textureHandle, type,
                                           _mipMaxLevel, _internalFormat,
                                           _width, _height, _numLayers);
#endif
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
                         GFXImageFormat internalFormat,
                         bool usePOW2) {
    bool isTextureLayer =
        (_textureData._textureType == TextureType::TEXTURE_2D_ARRAY &&
         target > 0);

    if (!isTextureLayer) {
        _format = GLUtil::GL_ENUM_TABLE::glImageFormatTable[to_uint(format)];
        _internalFormat =
            internalFormat == GFXImageFormat::DEPTH_COMPONENT
                ? GL_DEPTH_COMPONENT32
                : GLUtil::GL_ENUM_TABLE::glImageFormatTable[to_uint(
                      internalFormat)];
        setMipMapRange(mipLevels.x, mipLevels.y);

        if (Config::Profile::USE_2x2_TEXTURES) {
            _width = _height = 2;
        } else {
            _width = dimensions.width;
            _height = dimensions.height;
        }
    } else {
        DIVIDE_ASSERT(
            _width == dimensions.width && _height == dimensions.height,
            "glTexture error: Invalid dimensions for texture array layer");
    }

    if (ptr) {
        assert(_bitDepth != 0);
        if (_textureData._textureType != TextureType::TEXTURE_CUBE_MAP &&
            usePOW2 && !isTextureLayer) {
            GLushort xSize2 = _width, ySize2 = _height;
            GLdouble xPow2 = log((GLdouble)xSize2) / log(2.0);
            GLdouble yPow2 = log((GLdouble)ySize2) / log(2.0);

            GLushort ixPow2 = (GLushort)xPow2;
            GLushort iyPow2 = (GLushort)yPow2;

            if (xPow2 != (GLdouble)ixPow2) {
                ixPow2++;
            }
            if (yPow2 != (GLdouble)iyPow2) {
                yPow2++;
            }
            xSize2 = 1 << ixPow2;
            ySize2 = 1 << iyPow2;
            _power2Size = (_width == xSize2) && (_height == ySize2);
        }
        if (!_allocatedStorage) {
            reserveStorage();
        }
        assert(_allocatedStorage);

        GL_API::setPixelPackUnpackAlignment();
        if (_textureData._textureType == TextureType::TEXTURE_2D_ARRAY) {
#ifdef GL_VERSION_4_5
            glTextureSubImage3D(_textureData._textureHandle,
                                GL_TEXTURE_2D_ARRAY, 0, 0, 0, target, _width,
                                _height, 1, _format, GL_UNSIGNED_BYTE, ptr);
#else
            gl44ext::glTextureSubImage3DEXT(
                _textureData._textureHandle, GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                target, _width, _height, 1, _format, GL_UNSIGNED_BYTE, ptr);
#endif
        } else {
#ifdef GL_VERSION_4_5
            glTextureSubImage2D(
                _textureData._textureHandle,
                _textureData._textureType == TextureType::TEXTURE_CUBE_MAP
                    ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + target
                    : GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
                          _textureData._textureType)],
                0, 0, 0, _width, _height, _format, GL_UNSIGNED_BYTE, ptr);
#else
            gl44ext::glTextureSubImage2DEXT(
                _textureData._textureHandle,
                _textureData._textureType == TextureType::TEXTURE_CUBE_MAP
                    ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + target
                    : GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
                          _textureData._textureType)],
                0, 0, 0, _width, _height, _format, GL_UNSIGNED_BYTE, ptr);
#endif
        }
        _mipMapsDirty = true;
        updateMipMaps();
    } else {
        if (!_allocatedStorage) {
            reserveStorage();
        }
        assert(_allocatedStorage);
    }

    DIVIDE_ASSERT(_width > 0 && _height > 0,
                  "glTexture error: Invalid texture dimensions!");
}

void glTexture::Bind(GLubyte unit) {
    updateSampler();
    updateMipMaps();
    GL_API::bindTexture(unit, _textureData._textureHandle,
                        GLUtil::GL_ENUM_TABLE::glTextureTypeTable[to_uint(
                            _textureData._textureType)],
                        _samplerHash);
}
};
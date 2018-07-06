#include "Platform/Video/OpenGL/Headers/glResources.h"
#include "Headers/glPixelBuffer.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/OpenGL/Buffers/Headers/glMemoryManager.h"

namespace Divide {

size_t glPixelBuffer::sizeOf(GLenum dataType) const {
    switch (_dataType) {
        case GL_FLOAT:
            return sizeof(GLfloat);
        case GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);
        case GL_UNSIGNED_INT:
            return sizeof(GLuint);
        case GL_UNSIGNED_SHORT:
            return sizeof(GLushort);
        case GL_BYTE:
            return sizeof(GLbyte);
        case GL_SHORT:
            return sizeof(GLshort);
        case GL_INT:
            return sizeof(GLint);
    };
    return 0;
}

glPixelBuffer::glPixelBuffer(PBType type) : PixelBuffer(type) {
    switch (_pbtype) {
        case PB_TEXTURE_1D:
            _textureType = GLUtil::GLenum_to_uint(GL_TEXTURE_1D);
            break;
        case PB_TEXTURE_2D:
            _textureType = GLUtil::GLenum_to_uint(GL_TEXTURE_2D);
            break;
        case PB_TEXTURE_3D:
            _textureType = GLUtil::GLenum_to_uint(GL_TEXTURE_3D);
            break;
        default:
            Console::errorfn(Locale::get("ERROR_PB_INVALID_TYPE"));
            break;
    };
}

void glPixelBuffer::Destroy() {
    if (_textureID > 0) {
        glDeleteTextures(1, &_textureID);
        _textureID = 0;
    }

    GLUtil::freeBuffer(_pixelBufferHandle);

    _width = 0;
    _height = 0;
    _depth = 0;
}

void* glPixelBuffer::Begin(GLubyte nFace) const {
    DIVIDE_ASSERT(
        nFace < 6,
        "glPixelBuffer error: Tried to map an invalid PBO texture's face!");

    GLenum textureTypeEnum = static_cast<GLenum>(_textureType);
    GL_API::bindTexture(0, _textureID, textureTypeEnum);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    switch (_pbtype) {
        case PB_TEXTURE_1D:
            glTexSubImage1D(textureTypeEnum, 0, 0, _width, _format, _dataType, 0);
            break;
        case PB_TEXTURE_2D:
            glTexSubImage2D(textureTypeEnum, 0, 0, 0, _width, _height, _format,
                            _dataType, 0);
            break;
        case PB_TEXTURE_3D:
            glTexSubImage3D(textureTypeEnum, 0, 0, 0, 0, _width, _height, _depth,
                            _format, _dataType, 0);
            break;
    };

    GLsizeiptr bufferSize = (_width * 4) * sizeOf(_dataType);
    switch (_pbtype) {
        case PB_TEXTURE_2D:
            bufferSize *= _height;
            break;
        case PB_TEXTURE_3D:
            bufferSize *= _height * _depth;
            break;
    };

    GLUtil::allocBuffer(_pixelBufferHandle, bufferSize, GL_STREAM_DRAW);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
}

void glPixelBuffer::End() const {
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);  // release the mapped buffer
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    GL_API::unbindTexture(0, static_cast<GLenum>(_textureType));
}

void glPixelBuffer::Bind(GLubyte unit) const {
    GL_API::bindTexture(unit, _textureID, static_cast<GLenum>(_textureType));
}

bool glPixelBuffer::Create(GLushort width, GLushort height, GLushort depth,
                           GFXImageFormat internalFormatEnum,
                           GFXImageFormat formatEnum,
                           GFXDataFormat dataTypeEnum) {
    GLenum textureTypeEnum = static_cast<GLenum>(_textureType);
    _internalFormat =
        GLUtil::GL_ENUM_TABLE::glImageFormatTable[internalFormatEnum];
    _format = GLUtil::GL_ENUM_TABLE::glImageFormatTable[formatEnum];
    _dataType = GLUtil::GL_ENUM_TABLE::glDataFormat[dataTypeEnum];

    Destroy();
    Console::printfn(Locale::get("GL_PB_GEN"), width, height);
    _width = width;
    _height = height;
    _depth = depth;
    GLuint size = _width;
    if (_pbtype != PB_TEXTURE_1D) size *= _height;
    if (_pbtype == PB_TEXTURE_3D) size *= _depth;

    size *= 4 /*channels*/;

    glGenTextures(1, &_textureID);
    GL_API::setPixelPackUnpackAlignment();
    GL_API::bindTexture(0, _textureID, textureTypeEnum);
    glTexParameteri(textureTypeEnum, GL_GENERATE_MIPMAP, 0);
    glTexParameteri(textureTypeEnum, GL_TEXTURE_MIN_FILTER,
                    GLUtil::GLenum_to_uint(GL_NEAREST));
    glTexParameteri(textureTypeEnum, GL_TEXTURE_MAG_FILTER,
                    GLUtil::GLenum_to_uint(GL_NEAREST));
    glTexParameteri(textureTypeEnum, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(textureTypeEnum, GL_TEXTURE_MAX_LEVEL, 1000);
    glTexParameteri(textureTypeEnum, GL_TEXTURE_WRAP_S,
                    GLUtil::GLenum_to_uint(GL_REPEAT));
    if (_pbtype != PB_TEXTURE_1D) {
        glTexParameteri(textureTypeEnum, GL_TEXTURE_WRAP_T,
                        GLUtil::GLenum_to_uint(GL_REPEAT));
    }
    if (_pbtype == PB_TEXTURE_3D) {
        glTexParameteri(textureTypeEnum, GL_TEXTURE_WRAP_R,
                        GLUtil::GLenum_to_uint(GL_REPEAT));
    }

    void* pixels = nullptr;

    switch (_dataType) {
        default:
            pixels = MemoryManager_NEW GLubyte[size];
            break;
        case GL_FLOAT:
            pixels = MemoryManager_NEW GLfloat[size];
            break;
        case GL_UNSIGNED_INT:
            pixels = MemoryManager_NEW GLuint[size];
            break;
        case GL_UNSIGNED_SHORT:
            pixels = MemoryManager_NEW GLushort[size];
            break;
        case GL_BYTE:
            pixels = MemoryManager_NEW GLbyte[size];
            break;
        case GL_SHORT:
            pixels = MemoryManager_NEW GLshort[size];
            break;
        case GL_INT:
            pixels = MemoryManager_NEW GLint[size];
            break;
    };

    memset(pixels, 0, size * sizeOf(_dataType));

    switch (_pbtype) {
        case PB_TEXTURE_1D:
            glTexImage1D(textureTypeEnum, 0,
                         GLUtil::GLenum_to_uint(_internalFormat), _width, 0,
                         _format, _dataType, pixels);
            break;
        case PB_TEXTURE_2D:
            glTexImage2D(textureTypeEnum, 0,
                         GLUtil::GLenum_to_uint(_internalFormat), _width,
                         _height, 0, _format, _dataType, pixels);
            break;
        case PB_TEXTURE_3D:
            glTexImage3D(textureTypeEnum, 0,
                         GLUtil::GLenum_to_uint(_internalFormat), _width,
                         _height, _depth, 0, _format, _dataType, pixels);
            break;
    };
    MemoryManager::DELETE_ARRAY(pixels);
    GL_API::unbindTexture(0, textureTypeEnum);

    GLUtil::allocBuffer(size * sizeOf(_dataType), GL_STREAM_DRAW, _pixelBufferHandle);
    return true;
}

void glPixelBuffer::updatePixels(const GLfloat* const pixels) {
    GL_API::bindTexture(0, _textureID, static_cast<GLenum>(_textureType));
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    switch (_pbtype) {
        case PB_TEXTURE_1D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER,
                         (_width * 4) * sizeOf(_dataType), NULL,
                         GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_2D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER,
                         (_width * _height * 4) * sizeOf(_dataType), NULL,
                         GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_3D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER,
                         (_width * _height * _depth * 4) * sizeOf(_dataType),
                         NULL, GL_STREAM_DRAW);
            break;
    };

    GLfloat* ptr = (GLfloat*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
        memcpy(ptr, pixels, _width * _height * 4 * sizeOf(_dataType));
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);  // release the mapped buffer
    }

    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
};
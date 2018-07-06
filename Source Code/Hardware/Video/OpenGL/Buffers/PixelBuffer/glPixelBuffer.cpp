#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glPixelBuffer.h"
#include "Hardware/Video/Headers/GFXDevice.h"

size_t glPixelBuffer::sizeOf(GLenum dataType) const{
    switch(_dataType){
        case GL_FLOAT: return sizeof(GLfloat);
        case GL_UNSIGNED_BYTE: return sizeof(GLubyte);
        case GL_UNSIGNED_INT: return sizeof(GLuint);
        case GL_UNSIGNED_SHORT: return sizeof(GLushort);
        case GL_BYTE: return sizeof(GLbyte);
        case GL_SHORT: return sizeof(GLshort);
        case GL_INT: return sizeof(GLint);
    };
    return 0;
}

glPixelBuffer::glPixelBuffer(PBType type) : PixelBuffer(type) {
    switch(_pbtype){
        case PB_TEXTURE_1D:
            _textureType = GL_TEXTURE_1D;
            break;
        case PB_TEXTURE_2D:
            _textureType = GL_TEXTURE_2D;
            break;
        case PB_TEXTURE_3D:
            _textureType = GL_TEXTURE_3D;
            break;
        default:
            ERROR_FN(Locale::get("ERROR_PB_INVALID_TYPE"));
            break;
    };
}

void glPixelBuffer::Destroy() {
    if(_textureId > 0){
        glDeleteTextures(1, &_textureId);
        _textureId = 0;
    }

    if(_pixelBufferHandle > 0){
        glDeleteBuffers(1, &_pixelBufferHandle);
        _pixelBufferHandle = 0;
    }
    _width = 0;
    _height = 0;
    _depth = 0;
}

GLvoid* glPixelBuffer::Begin(GLubyte nFace) const {
    DIVIDE_ASSERT(nFace < 6, "glPixelBuffer error: Tried to map an invalid PBO texture's face!");

    GL_API::bindTexture(0, _textureId, _textureType);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    switch(_pbtype){
        case PB_TEXTURE_1D:
            glTexSubImage1D(_textureType, 0, 0, _width, _format, _dataType, 0);
            break;
        case PB_TEXTURE_2D:
            glTexSubImage2D(_textureType, 0, 0, 0, _width, _height, _format, _dataType, 0);
            break;
        case PB_TEXTURE_3D:
            glTexSubImage3D(_textureType, 0, 0, 0, 0, _width, _height, _depth, _format, _dataType, 0);
            break;
    };

    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    switch(_pbtype){
        case PB_TEXTURE_1D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_2D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_3D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*_depth*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
    };

    return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
}

void glPixelBuffer::End() const {

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    GL_API::unbindTexture(0, _textureType);
}

void glPixelBuffer::Bind(GLubyte unit) const {
    GL_API::bindTexture(unit, _textureId, _textureType);
}

bool glPixelBuffer::Create(GLushort width, GLushort height,GLushort depth, GFXImageFormat internalFormatEnum, GFXImageFormat formatEnum,GFXDataFormat dataTypeEnum) {
    _internalFormat = glImageFormatTable[internalFormatEnum];
    _format         = glImageFormatTable[formatEnum];
    _dataType       = glDataFormat[dataTypeEnum];

    Destroy();
    PRINT_FN(Locale::get("GL_PB_GEN"),width,height);
    _width = width;
    _height = height;
    _depth = depth;
    GLuint size = _width;
    if(_pbtype != PB_TEXTURE_1D)
        size *= _height;
    if(_pbtype == PB_TEXTURE_3D)
        size *= _depth;

    size *= 4/*channels*/;

    glGenTextures(1, &_textureId);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GL_API::bindTexture(0, _textureId, _textureType);
    glTexParameteri(_textureType, GL_GENERATE_MIPMAP, GL_FALSE);
    glTexParameteri(_textureType,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(_textureType,GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL, 1000);
    glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, GL_REPEAT);
    if(_pbtype != PB_TEXTURE_1D){
        glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    if(_pbtype == PB_TEXTURE_3D){
        glTexParameteri(_textureType, GL_TEXTURE_WRAP_R, GL_REPEAT);
    }

    void *pixels = nullptr;

    switch(_dataType){
        default: pixels = New GLubyte[size]; break;
        case GL_FLOAT: pixels = New GLfloat[size]; break;
        case GL_UNSIGNED_INT: pixels = New GLuint[size]; break;
        case GL_UNSIGNED_SHORT: pixels = New GLushort[size]; break;
        case GL_BYTE: pixels = New GLbyte[size]; break;
        case GL_SHORT: pixels = New GLshort[size]; break;
        case GL_INT: pixels = New GLint[size]; break;
    };

    memset(pixels, 0, size * sizeOf(_dataType) );

    switch(_pbtype){
        case PB_TEXTURE_1D:
            glTexImage1D(_textureType, 0, _internalFormat, _width, 0, _format, _dataType, pixels);
            break;
        case PB_TEXTURE_2D:
            glTexImage2D(_textureType, 0, _internalFormat, _width, _height, 0, _format, _dataType, pixels);
            break;
        case PB_TEXTURE_3D:
            glTexImage3D(_textureType, 0, _internalFormat, _width, _height,_depth, 0, _format, _dataType, pixels);
            break;
    };
    SAFE_DELETE_ARRAY(pixels);

    glGenBuffers(1, &_pixelBufferHandle);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, size * sizeOf(_dataType), 0, GL_STREAM_DRAW);

    GL_API::unbindTexture(0, _textureType);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    return true;
}

void glPixelBuffer::updatePixels(const GLfloat * const pixels) {
    GL_API::bindTexture(0, _textureId, _textureType);
    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    switch(_pbtype){
        case PB_TEXTURE_1D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_2D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
        case PB_TEXTURE_3D:
            glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*_depth*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW);
            break;
    };

    GLfloat* ptr = (GLfloat*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    if(ptr)	{
        memcpy(ptr, pixels, _width * _height * 4 * sizeOf(_dataType));
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
    }

    GL_API::setActiveBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}
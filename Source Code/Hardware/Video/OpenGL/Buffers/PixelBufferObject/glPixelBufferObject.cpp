#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "core.h"
#include "Headers/glPixelBufferObject.h"
#include "Hardware/Video/Headers/GFXDevice.h"

size_t glPixelBufferObject::sizeOf(GLenum dataType) const{
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

glPixelBufferObject::glPixelBufferObject(PBOType type) : PixelBufferObject(type) {
	switch(_pbotype){
		case PBO_TEXTURE_1D:
			_textureType = GL_TEXTURE_1D;
			break;
		case PBO_TEXTURE_2D:
			_textureType = GL_TEXTURE_2D;
			break;
		case PBO_TEXTURE_3D:
			_textureType = GL_TEXTURE_3D;
			break;
		default:
			ERROR_FN(Locale::get("ERROR_PBO_INVALID_TYPE"));
			break;
	};
}

void glPixelBufferObject::Destroy() {	
	if(_textureId > 0){
		GLCheck(glDeleteTextures(1, &_textureId));
		_textureId = 0;
	}

	if(_pixelBufferHandle > 0){
		GLCheck(glDeleteBuffers(1, &_pixelBufferHandle));
		_pixelBufferHandle = 0;
	}
	_width = 0;
	_height = 0;
	_depth = 0;
}

GLvoid* glPixelBufferObject::Begin(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glBindTexture(_textureType, _textureId));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	switch(_pbotype){
		case PBO_TEXTURE_1D:
			GLCheck(glTexSubImage1D(_textureType, 0, 0, _width, _format, _dataType, 0));
			break;
		case PBO_TEXTURE_2D:
			GLCheck(glTexSubImage2D(_textureType, 0, 0, 0, _width, _height, _format, _dataType, 0));
			break;
		case PBO_TEXTURE_3D:
			GLCheck(glTexSubImage3D(_textureType, 0, 0, 0, 0, _width, _height, _depth, _format, _dataType, 0));
			break;
	};

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	switch(_pbotype){
		case PBO_TEXTURE_1D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
		case PBO_TEXTURE_2D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
		case PBO_TEXTURE_3D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*_depth*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
	};
	
	return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
}

void glPixelBufferObject::End(GLubyte nFace) const {
	assert(nFace<6);
	GLCheck(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER)); // release the mapped buffer
	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    GLCheck(glBindTexture(_textureType, 0));
}

void glPixelBufferObject::Bind(GLubyte unit) const {
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_textureType, _textureId));
}

void glPixelBufferObject::Unbind(GLubyte unit) const {
    GL_API::setActiveTextureUnit(unit);
	GLCheck(glBindTexture(_textureType, 0 ));
}


bool glPixelBufferObject::Create(GLushort width, GLushort height,GLushort depth, GFXImageFormat internalFormatEnum, GFXImageFormat formatEnum,GFXDataFormat dataTypeEnum) {

	_internalFormat = glImageFormatTable[internalFormatEnum];
	_format         = glImageFormatTable[formatEnum];
	_dataType       = glDataFormat[dataTypeEnum];

	Destroy();
	PRINT_FN(Locale::get("GL_PBO_GEN"),width,height);
	_width = width;
	_height = height;
	_depth = depth;
	GLuint size = _width;
	if(_pbotype != PBO_TEXTURE_1D)
		size *= _height;
	if(_pbotype == PBO_TEXTURE_3D)
		size *= _depth;

	size *= 4/*channels*/;

	GLCheck(glGenTextures(1, &_textureId));
	GLCheck(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));	
	GLCheck(glBindTexture(_textureType, _textureId));	
	GLCheck(glTexParameteri(_textureType, GL_GENERATE_MIPMAP, GL_FALSE));
	GLCheck(glTexParameteri(_textureType,GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCheck(glTexParameteri(_textureType,GL_TEXTURE_MAG_FILTER, GL_NEAREST));	
	GLCheck(glTexParameteri(_textureType, GL_TEXTURE_BASE_LEVEL, 0));
	GLCheck(glTexParameteri(_textureType, GL_TEXTURE_MAX_LEVEL, 1000));
	GLCheck(glTexParameteri(_textureType, GL_TEXTURE_WRAP_S, GL_REPEAT));
	if(_pbotype != PBO_TEXTURE_1D){
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_WRAP_T, GL_REPEAT));
	}
	if(_pbotype == PBO_TEXTURE_3D){
		GLCheck(glTexParameteri(_textureType, GL_TEXTURE_WRAP_R, GL_REPEAT));
	}

	void *pixels = NULL;

	switch(_dataType){
		case GL_FLOAT: pixels = New GLfloat[size]; break;
		case GL_UNSIGNED_BYTE: pixels = New GLubyte[size]; break;
		case GL_UNSIGNED_INT: pixels = New GLuint[size]; break;
		case GL_UNSIGNED_SHORT: pixels = New GLushort[size]; break;
		case GL_BYTE: pixels = New GLbyte[size]; break;
		case GL_SHORT: pixels = New GLshort[size]; break;
		case GL_INT: pixels = New GLint[size]; break;
	};

	memset(pixels, 0, size * sizeOf(_dataType) );

	switch(_pbotype){
		case PBO_TEXTURE_1D:
			GLCheck(glTexImage1D(_textureType, 0, _internalFormat, _width, 0, _format, _dataType, pixels));
			break;
		case PBO_TEXTURE_2D:
			GLCheck(glTexImage2D(_textureType, 0, _internalFormat, _width, _height, 0, _format, _dataType, pixels));
			break;
		case PBO_TEXTURE_3D:
			GLCheck(glTexImage3D(_textureType, 0, _internalFormat, _width, _height,_depth, 0, _format, _dataType, pixels));
			break;
	};
	SAFE_DELETE_ARRAY(pixels);

    GLCheck(glGenBuffers(1, &_pixelBufferHandle));
    GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
    GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, size * sizeOf(_dataType), 0, GL_STREAM_DRAW));
	
	GLCheck(glBindTexture(_textureType, 0));	
	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	return true;
}


void glPixelBufferObject::updatePixels(const GLfloat * const pixels) {
	GLCheck(glBindTexture(_textureType, _textureId));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	switch(_pbotype){
		case PBO_TEXTURE_1D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
		case PBO_TEXTURE_2D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
		case PBO_TEXTURE_3D:
			GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*_depth*4) * sizeOf(_dataType), 0, GL_STREAM_DRAW));
			break;
	};

	
	GLfloat* ptr;
	GLCheck(ptr = (GLfloat*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
	if(ptr)	{
		memcpy(ptr, pixels, _width * _height * 4 * sizeOf(_dataType));
		GLCheck(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER)); // release the mapped buffer
	}

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
}

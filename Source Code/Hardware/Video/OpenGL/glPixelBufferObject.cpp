#include "glResources.h"
#include "core.h"
#include "glPixelBufferObject.h"
#include "Hardware/Video/GFXDevice.h"

glPixelBufferObject::glPixelBufferObject()
{
	if(!glewIsSupported("glBindBuffer"))
	{
		glBindBuffer = GLEW_GET_FUN(__glewBindBufferARB);
	}
	_textureType = GL_TEXTURE_2D;
	_pixelBufferHandle=0;
	_width = 0;
	_height = 0;
}

void glPixelBufferObject::Destroy()
{
	GLCheck(glDeleteTextures(1, &_textureId));
	GLCheck(glDeleteBuffers(1, &_pixelBufferHandle));
	_width = 0;
	_height = 0;
}


void* glPixelBufferObject::Begin(U8 nFace) const
{
	assert(nFace<6);
	GLCheck(glBindTexture(_textureType, _textureId));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	GLCheck(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGB, GL_FLOAT, 0));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeof(F32), 0, GL_STREAM_DRAW));
	return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	
	
}

void glPixelBufferObject::End(U8 nFace) const
{
	assert(nFace<6);
	GLCheck(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER)); // release the mapped buffer
	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
}

void glPixelBufferObject::Bind(U8 unit) const
{
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
	GLCheck(glEnable(_textureType));
	GLCheck(glBindTexture(_textureType, _textureId));

}

void glPixelBufferObject::Unbind(U8 unit) const
{
	GLCheck(glActiveTexture(GL_TEXTURE0 + unit));
	GLCheck(glBindTexture( _textureType, 0 ));
	GLCheck(glDisable(_textureType));
}


bool glPixelBufferObject::Create(U16 width, U16 height)
{
	Destroy();
	PRINT_FN("Generating pixelbuffer of dimmensions [%d x %d]",width,height);
	_width = width;
	_height = height;
	U32 size = _width * _height * 4/*channels*/;

	GLCheck(glGenTextures(1, &_textureId));
	GLCheck(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));	
	GLCheck(glBindTexture(GL_TEXTURE_2D, _textureId));	
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST));
	GLCheck(glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST));	
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NONE));
	GLCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NONE));
	GLCheck(glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));

	F32 *pixels = New F32[size];
	memset(pixels, 0, size * sizeof(F32) );
	GLCheck(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _width, _height, 0, GL_RGB, GL_FLOAT, pixels));
	SAFE_DELETE_ARRAY(pixels);

    GLCheck(glGenBuffers(1, &_pixelBufferHandle));
    GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
    GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, size * sizeof(F32), 0, GL_STREAM_DRAW));
	
	GLCheck(glBindTexture(GL_TEXTURE_2D, 0));	
	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	return true;
}




void glPixelBufferObject::updatePixels(const F32 * const pixels){
	GLCheck(glBindTexture(_textureType, _textureId));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	GLCheck(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGB, GL_FLOAT, 0));

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle));
	GLCheck(glBufferData(GL_PIXEL_UNPACK_BUFFER, _width * _height * 4 * sizeof(F32), 0, GL_STREAM_DRAW));

	F32* ptr = (F32*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if(ptr)
	{
		memcpy(ptr, pixels, _width * _height * 4 * sizeof(F32));
		GLCheck(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER)); // release the mapped buffer
	}

	GLCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
}

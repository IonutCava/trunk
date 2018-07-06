#include "glResources.h"
#include "resource.h"
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
	glDeleteTextures(1, &_textureId);
	glDeleteBuffers(1, &_pixelBufferHandle);
	_width = 0;
	_height = 0;
}


void* glPixelBufferObject::Begin(U8 nFace) const
{
	assert(nFace<6);
	glBindTexture(_textureType, _textureId);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGB, GL_FLOAT, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, (_width*_height*4) * sizeof(F32), 0, GL_STREAM_DRAW);
	return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	
	
}

void glPixelBufferObject::End(U8 nFace) const
{
	assert(nFace<6);
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void glPixelBufferObject::Bind(U8 unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glEnable(_textureType);
	glBindTexture(_textureType, _textureId);

}

void glPixelBufferObject::Unbind(U8 unit) const
{
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture( _textureType, 0 );
	glDisable(_textureType);
}


bool glPixelBufferObject::Create(U16 width, U16 height)
{
	Destroy();
	Con::getInstance().printfn("Generating pixelbuffer of dimmensions [%d x %d]",width,height);
	_width = width;
	_height = height;
	U32 size = _width * _height * 4/*channels*/;

	glGenTextures(1, &_textureId);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	
	glBindTexture(GL_TEXTURE_2D, _textureId);	
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_NEAREST);	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_NONE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_NONE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	F32 *pixels = new F32[size];
	memset(pixels, 0, size * sizeof(F32) );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _width, _height, 0, GL_RGB, GL_FLOAT, pixels);
	delete [] pixels;
	pixels = NULL;

    glGenBuffers(1, &_pixelBufferHandle);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, size * sizeof(F32), 0, GL_STREAM_DRAW);
	
	glBindTexture(GL_TEXTURE_2D, 0);	
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	return true;
}




void glPixelBufferObject::updatePixels(const F32 * const pixels){
	glBindTexture(_textureType, _textureId);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, GL_RGB, GL_FLOAT, 0);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBufferHandle);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, _width * _height * 4 * sizeof(F32), 0, GL_STREAM_DRAW);

	F32* ptr = (F32*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	if(ptr)
	{
		memcpy(ptr, pixels, _width * _height * 4 * sizeof(F32));
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release the mapped buffer
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

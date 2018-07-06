/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _GL_PIXEL_BUFFER_OBJECT_H
#define _GL_PIXEL_BUFFER_OBJECT_H

#include "Hardware/Video/Buffers/PixelBufferObject/Headers/PixelBufferObject.h"

class glPixelBufferObject : public PixelBufferObject {
public:

	glPixelBufferObject(PBOType type);
	~glPixelBufferObject() {Destroy();}

	bool Create(GLushort width, GLushort height,GLushort depth = 0,
				GFXImageFormat internalFormatEnum = RGBA8,
				GFXImageFormat formatEnum = RGBA,
				GFXDataFormat dataTypeEnum = FLOAT_32);

	void Destroy();

	GLvoid* Begin(GLubyte nFace=0) const;
	void End(GLubyte nFace=0) const;

	void Bind(GLubyte unit=0) const;
	void Unbind(GLubyte unit=0) const;

	void updatePixels(const GLfloat * const pixels);

private:
	bool checkStatus();
	size_t sizeOf(GLenum dataType) const;
private:
	GLenum _dataType;
	GLenum _format;
	GLenum _internalFormat;
};

#endif

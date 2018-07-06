/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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


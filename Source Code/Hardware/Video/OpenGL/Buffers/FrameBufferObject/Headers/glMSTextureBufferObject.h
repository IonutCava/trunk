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

#ifndef _GL_MS_TEXTURE_BUFFER_OBJECT_H
#define _GL_MS_TEXTURE_BUFFER_OBJECT_H

#include "glFrameBufferObject.h"
//Multisampled version of the glTextureBufferObject
class glMSTextureBufferObject : public glFrameBufferObject {

public:

	glMSTextureBufferObject();
	~glMSTextureBufferObject() {Destroy();}
	
	bool Create(GLushort width, GLushort height, GLubyte imageLayers = 0);

	void Destroy();

	void End(GLubyte nFace=0) const;		

private:
	GLuint _colorBufferHandle;
	GLuint _msaaBufferResolver;
	GLuint _msaaSamples;
};

#endif


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

#ifndef _GL_FRAME_BUFFER_OBJECT_H
#define _GL_FRAME_BUFFER_OBJECT_H
#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Hardware/Video/Buffers/FrameBufferObject/Headers/FrameBufferObject.h"

class glFrameBufferObject : public FrameBufferObject {

public:

	glFrameBufferObject(FBOType type) : FrameBufferObject(type)
	{
		_textureId[0] = 0;
		_textureId[1] = 0;
		_textureId[2] = 0;
		_textureId[3] = 0;
		_depthId      = 0;
		_imageLayers  = 0;
	}
	virtual ~glFrameBufferObject() {}
	
	virtual bool Create(GLushort width, GLushort height, GLubyte imageLayers = 0);
	virtual void Destroy();
	virtual void DrawToLayer(GLubyte face, GLubyte layer) {} ///<Use by multilayerd FBO's

	virtual void Begin(GLubyte nFace=0) const;	
	virtual void End(GLubyte nFace=0) const;
	virtual void Bind(GLubyte unit=0, GLubyte texture = 0);		
	virtual void Unbind(GLubyte unit=0);	

	void BlitFrom(FrameBufferObject* inputFBO);

protected:
	bool checkStatus();

protected:
	GLuint _textureId[4];  ///<Color attachements
	GLuint _depthId;      ///<Depth attachement
	GLuint _imageLayers;
};

#endif


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

#ifndef GL_UNIFORM_BUFFER_OBJECT_H_
#define GL_UNIFORM_BUFFER_OBJECT_H_

#include "Hardware/Video/OpenGL/Headers/glResources.h"
#include "Utility/Headers/GUIDWrapper.h"
#include "Utility/Headers/Vector.h"

enum UBO_NAME {
	Matrices_UBO  = 0,
	Materials_UBO = 1,
	Lights_UBO    = 2,
	UBO_PLACEHOLDER = 3
};

///Base class for shader uniform blocks
class glUniformBufferObject : public GUIDWrapper {
public:
	glUniformBufferObject();
	~glUniformBufferObject();
    ///Create a new buffer object to hold our uniform shader data
    ///if "dynamic" is false, the buffer will be created using GL_STATIC_DRAW
    ///if "dynamic" is true, the buffer will use eiter GL_STREAM_DRAW or GL_DYNAMIC_DRAW depending on the "stream" param
    ///default value will be a GL_DYNAMIC_DRAW, as most data will change once every few frames
    ///(lights might change per frame, so stream will be better in that case)
	void Create(GLint bufferIndex, bool dynamic = true, bool stream = false);
	///Reserve primitiveCount * implementation specific primitive size of space in the buffer and fill it with NULL values
	virtual void ReserveBuffer(GLuint primitiveCount, GLsizeiptr primitiveSize);
	virtual void ChangeSubData(GLintptr offset,	GLsizeiptr size, const GLvoid *data);
	virtual bool bindUniform(GLuint shaderProgramHandle, GLuint uboLocation);
	virtual bool bindBufferRange(GLintptr offset, GLsizeiptr size);
	virtual bool bindBufferBase();
	static GLuint getBindingIndice();

protected:
	static vectorImpl<GLuint> _bindingIndices;
	GLuint _UBOid;
    GLenum _usage;
	GLuint _bindIndex;
};
#endif
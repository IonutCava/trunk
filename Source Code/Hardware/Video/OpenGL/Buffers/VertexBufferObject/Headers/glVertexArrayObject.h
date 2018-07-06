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

#ifndef _GL_VERTEX_ARRAY_OBJECT_H
#define _GL_VERTEX_ARRAY_OBJECT_H

#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"
///For OpenGL 3.x we use Vertex Array Objects as our VBO implementation. If that fails, we fallback to simple Vertex Arrays
///One VAO contains: one VBO for data, one IBO for indices and vertex attribs for 
///- Vertex Data
///- Normals
///- TexCoords
///- Tangents
///- BiTangents
///- Bone weights
///- Bone indices
class glVertexArrayObject : public VertexBufferObject {

public:
	bool        Create(bool staticDraw = true);

	void		Destroy();

	
	void		Enable();
	void		Disable();
	void        Draw(PrimitiveType type, U8 LODindex = 0);
	///Never call Refresh() just queue it and the data will update before drawing
	bool queueRefresh();
	
	glVertexArrayObject();
	~glVertexArrayObject() {Destroy();}
	void setShaderProgram(ShaderProgram* const shaderProgram);

private:
	bool Refresh();
	/// Internally create the VBO
	bool CreateInternal();
	void Enable_VA();	
	void Enable_VBO();	
	void Enable_Shader_VBO();
	void Disable_VA();	
	void Disable_VBO();
	void Disable_Shader_VBO();
	void checkStatus();
private:
	GLuint _VAOid;

	bool _useVA;   ///<Fallback to Vertex Arrays if VBO creation fails
	bool _created; ///< VBO's can be auto-created as GL_STATIC_DRAW if Enable() is called before Create();
				   ///< This helps with multi-threaded asset loading without creating separate GL contexts for each thread
	bool _animationData;     ///< Used to bind an extra set of vertex attributes for bone indices and bone weights
	bool _refreshQueued;     ///< A refresh call might be called before "Create()". This should help with that

	vectorImpl<vec4<GLhalf> >  _normalsSmall;
	vectorImpl<vec4<GLshort> > _tangentSmall;
	vectorImpl<vec4<GLshort> > _bitangentSmall;
};

#endif


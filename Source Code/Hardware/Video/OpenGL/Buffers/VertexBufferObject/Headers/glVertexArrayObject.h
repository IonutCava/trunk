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
#include "Hardware/Video/OpenGL/Headers/glResources.h"
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
	virtual bool Create(bool staticDraw = true);
	virtual void Destroy();
	virtual void Enable();
	virtual void Disable();
	virtual void Draw(U8 LODindex = 0);
    virtual void Draw(GFXDataFormat f, U32 count, const void* first_element);
	virtual ~glVertexArrayObject() {Destroy();}

    glVertexArrayObject(PrimitiveType type);
    ///Never call Refresh() just queue it and the data will update before drawing
	bool queueRefresh();
	void setShaderProgram(ShaderProgram* const shaderProgram);

protected:
    virtual bool computeTriangleList();
	/// If we have a shader, we create a VAO, if not, we use simple VBO + IBO. If that fails, use VA
	virtual bool Refresh();
	/// Internally create the VBO
	virtual bool CreateInternal();
	/// Enable Vertex Array Data
	virtual void Enable_VA();	
	/// Disable Vertex Array Data
	virtual void Disable_VA();	
	/// Enable VBO Data (only vertex and normal pointers)
	virtual void Enable_VBO();
	/// Disable VBO Data (only vertex and normal pointers)
	virtual void Disable_VBO();
	/// Enable texCoord pointers manually
	virtual void Enable_VBO_TexPointers();
	/// Dsiable texCoord pointers manually to set client texture states to defaults
	virtual void Disable_VBO_TexPointers();
	/// Enable full VAO based VBO (all pointers are tracked by VAO's)
	virtual void Enable_Shader_VBO();
	/// Disable full VAO based VBO
	virtual void Disable_Shader_VBO();
	/// Integrity checks
	void checkStatus();

protected:

	GLuint _VAOid;
    GLuint _DepthVAOid;

	bool _useVA;   ///<Fallback to Vertex Arrays if VBO creation fails
	bool _created; ///< VBO's can be auto-created as GL_STATIC_DRAW if Enable() is called before Create();
				   ///< This helps with multi-threaded asset loading without creating separate GL contexts for each thread
	bool _animationData;     ///< Used to bind an extra set of vertex attributes for bone indices and bone weights
	bool _refreshQueued;     ///< A refresh call might be called before "Create()". This should help with that
    bool _bound;
	vectorImpl<vec4<GLhalf> >  _normalsSmall;
	vectorImpl<vec4<GLshort> > _tangentSmall;
	vectorImpl<vec4<GLshort> > _bitangentSmall;
};

#endif


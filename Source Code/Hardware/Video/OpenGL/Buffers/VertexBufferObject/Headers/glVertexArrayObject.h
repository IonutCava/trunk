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
///Always bind a shader, even a dummy one when rendering geometry. No more fixed matrix API means no more VBOs or VAs
///One VAO contains: one VBO for data, one IBO for indices and uploads to the shader vertex attribs for:
///- Vertex Data  bound to location Divide::GL::VERTEX_POSITION_LOCATION
///- Normals      bound to location Divide::GL::VERTEX_NORMAL_LOCATION
///- TexCoords    bound to location Divide::GL::VERTEX_TEXCOORD_LOCATION
///- Tangents     bound to location Divide::GL::VERTEX_TANGENT_LOCATION
///- BiTangents   bound to location Divide::GL::VERTEX_BITANGENT_LOCATION
///- Bone weights bound to location Divide::GL::VERTEX_BONE_WEIGHT_LOCATION
///- Bone indices bound to location Divide::GL::VERTEX_BONE_INDICE_LOCATION

class glVertexArrayObject : public VertexBufferObject {
public:
	glVertexArrayObject(const PrimitiveType& type);
	virtual ~glVertexArrayObject();

    //Shader manipulation to replace the fixed pipeline. Always specify a shader before the draw calls!
    void setShaderProgram(ShaderProgram* const shaderProgram);

	virtual bool Create(bool staticDraw = true);
	virtual void Destroy();

	virtual bool SetActive();

	virtual void Draw(const U8 LODindex = 0);
    virtual void DrawRange();

    ///Never call Refresh() just queue it and the data will update before drawing
	inline bool queueRefresh() {_refreshQueued = true; return true;}

protected:
    virtual bool computeTriangleList();
	/// If we have a shader, we create a VAO, if not, we use simple VBO + IBO. If that fails, use VA
	virtual bool Refresh();
	/// Internally create the VBO
	virtual bool CreateInternal();
	/// Enable full VAO based VBO (all pointers are tracked by VAO's)
	virtual void Upload_VBO_Attributes();
	virtual void Upload_VBO_Depth_Attributes();
	/// Integrity checks
	void checkStatus();

protected:

	GLuint _VAOid;
    GLuint _DepthVAOid;
	GLuint _usage;
	bool _animationData;     ///< Used to bind an extra set of vertex attributes for bone indices and bone weights
	bool _refreshQueued;     ///< A refresh call might be called before "Create()". This should help with that
 	vectorImpl<vec4<GLhalf> >  _normalsSmall;
	vectorImpl<vec4<GLshort> > _tangentSmall;
	vectorImpl<vec4<GLshort> > _bitangentSmall;
};

#endif

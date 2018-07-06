/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _GL_VERTEX_BUFFER_OBJECT_H
#define _GL_VERTEX_BUFFER_OBJECT_H

#include "../VertexBufferObject.h"

class glVertexBufferObject : public VertexBufferObject {

public:
	bool        Create(bool staticDraw = true);

	void		Destroy();

	
	void		Enable();
	void		Disable();

	bool Refresh();

	glVertexBufferObject();
	~glVertexBufferObject() {Destroy();}
	void setShaderProgram(ShaderProgram* const shaderProgram);

private:
	/// Internally create the VBO
	bool CreateInternal();
	void Enable_VA();	
	void Enable_VBO();	
	void Enable_Shader_VBO();
	void Disable_VA();	
	void Disable_VBO();
	void Disable_Shader_VBO();

	///Bone data
	void Enable_Bone_Data_VA();
	void Enable_Bone_Data_VBO();
	void Enable_Shader_Bone_Data_VBO();
	void Disable_Bone_Data_VA();
	void Disable_Bone_Data_VBO();
	void Disable_Shader_Bone_Data_VBO();

private:
	bool _created; ///< VBO's can be auto-created as GL_STATIC_DRAW if Enable() is called before Create();
				   ///< Tis helps with multi-threaded asset loading without creating separate GL contexts for each thread
	bool _animationData;     ///< Used to bind an extra set of vertex attributes for bone indices and bone weights
	I32 _positionDataLocation;
	I32 _normalDataLocation;
	I32 _texCoordDataLocation;
	I32 _tangentDataLocation;
	I32 _biTangentDataLocation;
	I32 _boneIndiceDataLocation;
	I32 _boneWeightDataLocation;

};

#endif


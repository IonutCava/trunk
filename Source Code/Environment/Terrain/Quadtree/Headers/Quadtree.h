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

#ifndef _QUAD_TREE
#define _QUAD_TREE

#include "core.h"

class QuadtreeNode;
class BoundingBox;
class Terrain;
class Transform;
class ShaderProgram;
class VertexBufferObject;
class Quadtree {
public:
	void Build(BoundingBox& terrainBBox, vec2<U32> HMSize, U32 minHMSize,VertexBufferObject* const groundVBO);
	BoundingBox& computeBoundingBox(const vectorImpl<vec3<F32> >& vertices);
	void Destroy();

	inline void setParentShaderProgram(ShaderProgram* const shaderProgram) {_parentShaderProgram = shaderProgram;}
    inline void setParentVBO(VertexBufferObject* const vbo) {_parentVBO = vbo;}

	void DrawGround(bool drawInReflection);
	void DrawGrass(U32 geometryIndex, Transform* const parentTransform);
	void DrawBBox();

	QuadtreeNode*	FindLeaf(const vec2<F32>& pos);

	Quadtree()	{_root = NULL; _parentShaderProgram = NULL; _parentVBO = NULL;}
	~Quadtree()	{Destroy();}

private:
	QuadtreeNode*	     _root;
	ShaderProgram*       _parentShaderProgram; //<Pointer to the terrain shader
    VertexBufferObject*  _parentVBO; //<Pointer to the terrain VBO
};

#endif

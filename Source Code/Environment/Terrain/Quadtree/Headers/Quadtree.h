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

#ifndef _QUAD_TREE
#define _QUAD_TREE

#include "resource.h"

class QuadtreeNode;
class BoundingBox;
class ivec2;
class Terrain;
class ShaderProgram;
class Quadtree {
public:
	void Build(BoundingBox& terrainBBox, ivec2 HMSize, U32 minHMSize);
	BoundingBox& computeBoundingBox(const std::vector<vec3>& vertices);
	void Destroy();

	inline ShaderProgram*     const     getParentShaderProgram()       {return _parentShaderProgram;}
	inline void                         setParentShaderProgram(ShaderProgram* const shaderProgram) {_parentShaderProgram = shaderProgram;}

	void DrawGround(bool drawInReflection);
	void DrawGrass();
	void DrawBBox();

	QuadtreeNode*	FindLeaf(vec2& pos);

	Quadtree()	{_root = NULL;}
	~Quadtree()	{Destroy();}

private:
	QuadtreeNode*	_root;	
	ShaderProgram*  _parentShaderProgram;
};

#endif


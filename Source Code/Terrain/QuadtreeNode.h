/*“Copyright 2009-2011 DIVIDE-Studio”*/
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

#ifndef _QUAD_TREE_NODE
#define _QUAD_TREE_NODE

#include "resource.h"
#include "Utility/Headers/BoundingBox.h"

#define CHILD_NW	0
#define CHILD_NE	1
#define CHILD_SW	2
#define CHILD_SE	3

#define CHUNK_BIT_TESTCHILDREN		0x1
#define CHUNK_BIT_WATERREFLECTION	0x2
#define CHUNK_BIT_DEPTHMAP			0x3


class Frustum;
class TerrainChunk;
class Terrain;
class QuadtreeNode {
public:
	//fonction recursive de traitement des noeuds
	void Build(U32 depth, ivec2 pos, ivec2 HMsize, U32 minHMSize);
	bool computeBoundingBox(const std::vector<vec3>& vertices);
	void Destroy();

	void DrawGround(I32 options);
	void DrawGrass(bool drawInReflection);
	void DrawTrees(bool drawInReflection);
	void DrawBBox();

	inline bool isALeaf() const							{return _children==0;}
	inline const BoundingBox&	getBoundingBox()		{return _boundingBox;}
	inline void setBoundingBox(const BoundingBox& bbox)	{_boundingBox = bbox;}
	inline QuadtreeNode*	getChildren()				{return _children;}
	inline TerrainChunk*	getChunk()					{return _terrainChunk;}

	QuadtreeNode()  {_children = NULL; _terrainChunk = NULL; _LOD = 0;}
	~QuadtreeNode() {Destroy();}

private:
	I32				_LOD;				// LOD level
	F32			    _camDistance;		// Distance to camera
	BoundingBox		_boundingBox;		// Node BoundingBox
	QuadtreeNode*	_children;			// Node children
	TerrainChunk*	_terrainChunk;		// Terrain Chunk contained in node
};

#endif

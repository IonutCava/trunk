/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _QUAD_TREE_NODE
#define _QUAD_TREE_NODE

#include "core.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

enum ChildPosition{
    CHILD_NW = 0,
    CHILD_NE = 1,
    CHILD_SW = 2,
    CHILD_SE = 3
};

enum ChunkBit{
    CHUNK_BIT_TESTCHILDREN	  = toBit(1),
    CHUNK_BIT_WATERREFLECTION = toBit(2),
    CHUNK_BIT_DEPTHMAP		  = toBit(3)
};

class Frustum;
class TerrainChunk;
class Terrain;
class Transform;
class ShaderProgram;
class VertexBufferObject;
class QuadtreeNode {
public:
    ///recursive node building function
    void Build(const U8 depth,const vec2<U32>& pos,const vec2<U32>& HMsize, U32 minHMSize, VertexBufferObject* const groundVBO, U32& chunkCount);
    bool computeBoundingBox(const vectorImpl<vec3<F32> >& vertices);
    void Destroy();

    inline void setParentShaderProgram(ShaderProgram* const shaderProgram) {_parentShaderProgram = shaderProgram;}

    void DrawGround(I32 options,VertexBufferObject* const terrainVBO);
    void DrawGrass(U32 geometryIndex, Transform* const parentTransform);
    void DrawBBox();
    void GenerateGrassIndexBuffer(U32 bilboardCount);

    inline bool isALeaf() const							{return _children==0;}
    inline BoundingBox&	getBoundingBox()         		{return _boundingBox;}
    inline void setBoundingBox(const BoundingBox& bbox)	{_boundingBox = bbox;}
    inline QuadtreeNode*	getChildren()				{return _children;}
    inline TerrainChunk*	getChunk()					{return _terrainChunk;}

    QuadtreeNode()  {_children = NULL; _terrainChunk = NULL; _LOD = 0;}
    ~QuadtreeNode() {Destroy();}

protected:
	bool  isInView(I32 options);

private:
    I8				_LOD;				///< LOD level
    F32			    _camDistance;		///< Distance to camera
    BoundingBox		_boundingBox;		///< Node BoundingBox
    QuadtreeNode*	_children;			///< Node children
    TerrainChunk*	_terrainChunk;		///< Terrain Chunk contained in node
    ShaderProgram*  _parentShaderProgram; ///< A reference to the terrain shader in case we need special rendering for this node
};

#endif

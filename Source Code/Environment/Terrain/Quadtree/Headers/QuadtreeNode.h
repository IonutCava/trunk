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
#include "Hardware/Video/Headers/RenderAPIWrapper.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

namespace Divide {

enum ChildPosition{
    CHILD_NW = 0,
    CHILD_NE = 1,
    CHILD_SW = 2,
    CHILD_SE = 3
};

enum ChunkBit{
    CHUNK_BIT_TESTCHILDREN = toBit(1),
    CHUNK_BIT_WATERREFLECTION = toBit(2),
    CHUNK_BIT_SHADOWMAP = toBit(3)
};

class Terrain;
class TerrainChunk;
class Transform;
class SceneState;
class VertexBuffer;
class ShaderProgram;
class SceneGraphNode;
class SceneRenderState;

class QuadtreeNode {
public:
    ///recursive node building function
    void Build( const U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, U32 minHMSize, Terrain* const terrain, U32& chunkCount );
    bool computeBoundingBox();
    void Destroy();

    void createDrawCommand(U32 options, const SceneRenderState& sceneRenderState, vectorImpl<GenericDrawCommand>& drawCommandsOut);
    void drawBBox() const;

    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);

    inline bool isALeaf() const							{ return !(_children[CHILD_NW] && _children[CHILD_NE] && _children[CHILD_SW] && _children[CHILD_SE]); }
    inline BoundingBox&	getBoundingBox()         		{return _boundingBox;}
    inline void setBoundingBox(const BoundingBox& bbox)	{_boundingBox = bbox;}
    inline TerrainChunk*	getChunk()					{return _terrainChunk;}

    inline QuadtreeNode*	getChild(ChildPosition pos) { return _children[pos]; }
    inline QuadtreeNode*	getChild(U32 index)         { return _children[index]; }

    QuadtreeNode();
    ~QuadtreeNode();

    inline U8 getLoD() const { return _LOD; }

protected:
    bool  isInView(U32 options, const SceneRenderState& sceneState) const;

private:
    F32             _terLoDOffset;      ///<Small offset to prevent wrong LoD selection on border cases
    U32             _minHMSize;
    I8				_LOD;				///< LOD level
    BoundingBox		_boundingBox;		///< Node BoundingBox
    BoundingSphere  _boundingSphere;    ///< Node BoundingSphere
    QuadtreeNode* 	_children[4];		///< Node children
    TerrainChunk*	_terrainChunk;		///< Terrain Chunk contained in node
};

}; //namespace Divide

#endif

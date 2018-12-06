/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _QUAD_TREE_NODE
#define _QUAD_TREE_NODE

#include "Platform/Video/Headers/RenderAPIWrapper.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingSphere.h"

namespace Divide {

enum class ChildPosition :U32 { 
    CHILD_NW = 0, 
    CHILD_NE = 1, 
    CHILD_SW = 2, 
    CHILD_SE = 3
};

enum class ChunkBit : U8 {
    CHUNK_BIT_TESTCHILDREN = toBit(1),
    CHUNK_BIT_WATERREFLECTION = toBit(2),
    CHUNK_BIT_SHADOWMAP = toBit(3)
};

class Terrain;
class TerrainChunk;
class SceneState;
class IMPrimitive;
class VertexBuffer;
class RenderPackage;
class ShaderProgram;
class SceneGraphNode;
class SceneRenderState;

class QuadtreeChildren;

class QuadtreeNode {
   public:
     QuadtreeNode();
     ~QuadtreeNode();

    /// recursive node building function
    void build(GFXDevice& context,
               const U8 depth,
               const vec2<U16>& pos,
               const vec2<U16>& HMsize,
               U32 targetChunkDimension,
               Terrain* const terrain,
               U32& chunkCount);

    bool computeBoundingBox();
    
    void drawBBox(GFXDevice& context, RenderPackage& packageOut);

    void sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn,
                     SceneState& sceneState);

    inline bool isALeaf() const { return _children == nullptr; }

    inline const BoundingBox& getBoundingBox() const { return _boundingBox; }
    inline void setBoundingBox(const BoundingBox& bbox) { _boundingBox = bbox; }
    inline TerrainChunk* getChunk() { return _terrainChunk.get(); }

    inline QuadtreeNode& getChild(ChildPosition pos) const;
    inline QuadtreeNode& getChild(U32 index) const;

   protected:
    bool isInView(U32 options, const SceneRenderState& sceneState) const;

   private:
    mutable I8 _frustPlaneCache;
    U32 _targetChunkDimension;
    BoundingBox _boundingBox;        ///< Node BoundingBox
    BoundingSphere _boundingSphere;  ///< Node BoundingSphere
    IMPrimitive*  _bbPrimitive;
    std::unique_ptr<QuadtreeChildren> _children;     ///< Node children
    std::unique_ptr<TerrainChunk> _terrainChunk;     ///< Terrain Chunk contained in node
};

class QuadtreeChildren {
private:
    friend class QuadtreeNode;

    typedef std::array<QuadtreeNode, 4> QuadtreeNodes;

    inline QuadtreeNodes& operator()() { return _nodes; }
    inline QuadtreeNode& operator[](ChildPosition pos) { return _nodes[to_U32(pos)]; }

    inline const QuadtreeNodes& operator()() const { return _nodes; }
    inline const QuadtreeNode& operator[](ChildPosition pos) const { return _nodes[to_U32(pos)]; }

    QuadtreeNodes _nodes;
};

inline QuadtreeNode& QuadtreeNode::getChild(ChildPosition pos) const {
    return (*_children)[pos];
}

inline QuadtreeNode& QuadtreeNode::getChild(U32 index) const {
    return (*_children)[static_cast<ChildPosition>(index)];
}

};  // namespace Divide

#endif

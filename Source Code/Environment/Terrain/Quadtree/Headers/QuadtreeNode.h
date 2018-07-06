/*
   Copyright (c) 2016 DIVIDE-Studio
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

enum class ChunkBit : U32 {
    CHUNK_BIT_TESTCHILDREN = toBit(1),
    CHUNK_BIT_WATERREFLECTION = toBit(2),
    CHUNK_BIT_SHADOWMAP = toBit(3)
};

class Terrain;
class TerrainChunk;
class Transform;
class SceneState;
class IMPrimitive;
class VertexBuffer;
class ShaderProgram;
class SceneGraphNode;
class SceneRenderState;

class QuadtreeNode {
   public:
    /// recursive node building function
    void Build(GFXDevice& context, const U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize,
               U32 minHMSize, Terrain* const terrain, U32& chunkCount);

    bool computeBoundingBox();
    
    void getBufferOffsetAndSize(U32 options,
                                const SceneRenderState& sceneRenderState,
                                vectorImpl<vec3<U32>>& chunkBufferData) const;
    void drawBBox(GFXDevice& context, GenericDrawCommands& commandsOut);

    void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                     SceneState& sceneState);

    inline bool isALeaf() const {
        return !(getChild(ChildPosition::CHILD_NW) && 
                 getChild(ChildPosition::CHILD_NE) &&
                 getChild(ChildPosition::CHILD_SW) && 
                 getChild(ChildPosition::CHILD_SE));
    }

    inline BoundingBox& getBoundingBox() { return _boundingBox; }
    inline void setBoundingBox(const BoundingBox& bbox) { _boundingBox = bbox; }
    inline TerrainChunk* getChunk() { return _terrainChunk; }

    inline QuadtreeNode* getChild(ChildPosition pos) const { return getChild(to_uint(pos)); }
    inline QuadtreeNode* getChild(U32 index) const { return _children[index]; }

    QuadtreeNode();
    ~QuadtreeNode();

    inline U8 getLoD() const { return _LOD; }

   protected:
    bool isInView(U32 options, const SceneRenderState& sceneState) const;

   private:
    F32 _terLoDOffset;  ///<Small offset to prevent wrong LoD selection on
                        ///border cases
    U32 _minHMSize;
    I8 _LOD;                         ///< LOD level
    BoundingBox _boundingBox;        ///< Node BoundingBox
    BoundingSphere _boundingSphere;  ///< Node BoundingSphere
    QuadtreeNode* _children[4];      ///< Node children
    TerrainChunk* _terrainChunk;     ///< Terrain Chunk contained in node
    IMPrimitive*  _bbPrimitive;
};

};  // namespace Divide

#endif

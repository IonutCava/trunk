/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _QUAD_TREE
#define _QUAD_TREE

#include "Core/Math/Headers/MathMatrices.h"
#include "Platform/Video/Headers/RenderAPIWrapper.h"

namespace Divide {

class Terrain;
class Transform;
class SceneState;
class BoundingBox;
class VertexBuffer;
class QuadtreeNode;
class ShaderProgram;
class SceneGraphNode;
class SceneRenderState;

class Quadtree {
   public:
    void Build(BoundingBox& terrainBBox, const vec2<U32>& HMSize, U32 minHMSize,
               Terrain* const terrain);
    BoundingBox& computeBoundingBox();

    inline U32 getChunkCount() const { return _chunkCount; }

    void sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn,
                     SceneState& sceneState);

    void getChunkBufferData(const SceneRenderState& sceneRenderState, 
                            vectorImpl<vec3<U32>>& chunkBufferData) const;

    void drawBBox() const;

    QuadtreeNode* findLeaf(const vec2<F32>& pos);

    Quadtree();
    ~Quadtree();

   private:
    U32 _chunkCount;
    VertexBuffer* _parentVB;  //<Pointer to the terrain VB

    std::unique_ptr<QuadtreeNode> _root;
};

};  // namespace Divide

#endif

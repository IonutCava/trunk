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

    void Build(BoundingBox& terrainBBox, vec2<U32>& HMSize, U32 minHMSize, VertexBufferObject* const groundVBO);
    BoundingBox& computeBoundingBox(const vectorImpl<vec3<F32> >& vertices);
    void Destroy();

	inline U32  getChunkCount() const { return _chunkCount;}
    inline void setParentShaderProgram(ShaderProgram* const shaderProgram) {_parentShaderProgram = shaderProgram;}
    inline void setParentVBO(VertexBufferObject* const vbo) {_parentVBO = vbo;}

    void DrawGround(bool drawInReflection);
    void DrawGrass(U32 geometryIndex, Transform* const parentTransform);
    void DrawBBox();
    void GenerateGrassIndexBuffer(U32 bilboardCount);

    QuadtreeNode*	FindLeaf(const vec2<F32>& pos);

    Quadtree()	{_root = NULL; _parentShaderProgram = NULL; _parentVBO = NULL; _chunkCount = 0;}
    ~Quadtree()	{Destroy();}

protected:
    void GenerateIndexBuffer(vec2<U32>& HMsize, VertexBufferObject* const groundVBO);

private:
	U32                  _chunkCount;
    QuadtreeNode*	     _root;
    ShaderProgram*       _parentShaderProgram; //<Pointer to the terrain shader
    VertexBufferObject*  _parentVBO; //<Pointer to the terrain VBO
};

#endif

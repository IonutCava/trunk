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

#ifndef _TERRAIN_CHUNK_H
#define _TERRAIN_CHUNK_H

#include "core.h"
#include "Hardware/Video/Buffers/VertexBufferObject/Headers/VertexBufferObject.h"

class Mesh;
class Terrain;
struct FileData;
class Transform;
class ShaderProgram;
class SceneGraphNode;

struct ChunkGrassData{
    vectorImpl<vectorImpl<U32 > > _grassIndices;
    vectorImpl<U32 >              _grassIndexOffset;
    vectorImpl<U32 >              _grassIndexSize;
    VertexBufferObject*           _grassVBO;
    F32                           _grassVisibility;
    inline bool empty() {return _grassIndices.empty();}

    ChunkGrassData() : _grassVBO(NULL)
    {
    }

    ~ChunkGrassData()
    {
		SAFE_DELETE(_grassVBO);
        _grassIndexOffset.clear();
        _grassIndexSize.clear();
    }
};

class TerrainChunk{
public:
    TerrainChunk() {}
    ~TerrainChunk() {Destroy();}
    void Load(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, VertexBufferObject* const vbo);
    void Destroy();

    I32  DrawGround(I8 lod,ShaderProgram* const program, VertexBufferObject* const vbo);
    void DrawGrass(I8 lod, F32 distance, U32 index, Transform* const parentTransform);
    
    void addObject(Mesh* obj);
    void addTree(const vec4<F32>& pos,F32 scale, const FileData& tree,SceneGraphNode* parentNode);

    void GenerateGrassIndexBuffer(U32 bilboardCount);

    inline F32 getMinHeight()              const {return _heightBounds[0];}
    inline F32 getMaxHeight()              const {return _heightBounds[1];}
    inline ChunkGrassData&  getGrassData()       {return _grassData;}

private:
    void ComputeIndicesArray(I8 lod, U8 depth,const vec2<U32>& position,const vec2<U32>& heightMapSize, VertexBufferObject* const vbo);

private:
    vectorImpl<U32> 	_indice[Config::TERRAIN_CHUNKS_LOD];
    U16					_indOffsetW[Config::TERRAIN_CHUNKS_LOD];
    U16  				_indOffsetH[Config::TERRAIN_CHUNKS_LOD];
    U32                 _lodIndOffset[Config::TERRAIN_CHUNKS_LOD];
    U32                 _lodIndCount[Config::TERRAIN_CHUNKS_LOD];
    U32                 _chunkIndOffset;
    ChunkGrassData      _grassData;
    F32                 _heightBounds[2]; //< 0 = minHeight, 1 = maxHeight
};

#endif

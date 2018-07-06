/*
   Copyright (c) 2013 DIVIDE-Studio
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

class Mesh;
class Terrain;
struct FileData;
class Transform;
class ShaderProgram;
class SceneGraphNode;
class VertexBufferObject;

struct ChunkGrassData{
    vectorImpl<vectorImpl<U32> > _grassIndice;
    VertexBufferObject* _grassVBO;
    F32                 _grassVisibility;
    inline bool empty(){return _grassIndice.empty();}

    ChunkGrassData() : _grassVBO(NULL)
    {
    }

    ~ChunkGrassData()
    {
        for(U8 i = 0; i < _grassIndice.size(); i++){
            _grassIndice[i].clear();
        }
        _grassIndice.clear();
    }
};

class TerrainChunk{
public:
	void Destroy();
	I32 DrawGround(I8 lod,ShaderProgram* const program, VertexBufferObject* const vbo);
	void DrawGrass(I8 lod, F32 d,U32 geometryIndex, Transform* const parentTransform);
	void Load(U8 depth, vec2<U32> pos, vec2<U32> HMsize,VertexBufferObject* const groundVBO);

	inline vectorImpl<U32>&			getIndiceArray(I8 lod)		   {return _indice[lod];}

    ChunkGrassData& getGrassData() {return _grassData;}
	void addObject(Mesh* obj);
	void addTree(const vec4<F32>& pos,F32 scale, const FileData& tree,SceneGraphNode* parentNode);

	TerrainChunk() {}
	~TerrainChunk() {Destroy();}

private:
	void ComputeIndicesArray(I8 lod, U8 depth,const vec2<U32>& position,const vec2<U32>& heightMapSize);

private:
	vectorImpl<U32> 	_indice[TERRAIN_CHUNKS_LOD];
	U16					_indOffsetW[TERRAIN_CHUNKS_LOD];
	U16  				_indOffsetH[TERRAIN_CHUNKS_LOD];
    ChunkGrassData      _grassData;
};

#endif

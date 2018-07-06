#ifndef TERRAINCHUNK_H
#define TERRAINCHUNK_H

#include "resource.h"
#include "Geometry/Object3DFlyWeight.h"
using namespace std;

#define TERRAIN_CHUNKS_LOD 3

#define TERRAIN_CHUNK_LOD0	300.0f
#define TERRAIN_CHUNK_LOD1	380.0f

class Mesh;
class TerrainChunk
{
public:
	void Destroy();
	int  DrawGround(U32 lod);
	void DrawGrass(U32 lod, F32 d);
	void DrawTrees(U32 lod, F32 d);
	void Load(U32 depth, ivec2 pos, ivec2 HMsize);

	inline std::vector<U32>&					getIndiceArray(U32 lod)	{return _indice[lod];}
	inline std::vector<U32>&					getGrassIndiceArray()	{return _grassIndice;}
	inline std::vector<Object3DFlyWeight* >&    getTreeArray()          {return _trees;}
	void								addObject(Mesh* obj);
	void								addTree(const vec3& pos, F32 rotation, F32 scale,const std::string& tree_shader, const FileData& tree);
	TerrainChunk() {}
	~TerrainChunk() {Destroy();}

private:
	void ComputeIndicesArray(U32 lod, U32 depth, ivec2 pos, ivec2 HMsize);

private:
	std::vector<U32> 	_indice[TERRAIN_CHUNKS_LOD];
	U32					_indOffsetW[TERRAIN_CHUNKS_LOD];
	U32					_indOffsetH[TERRAIN_CHUNKS_LOD];

	std::vector<U32>    _grassIndice;

	std::vector<Object3DFlyWeight* >           _trees;

	std::string				    previousModel;
};

#endif


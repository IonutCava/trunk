#ifndef TERRAINCHUNK_H
#define TERRAINCHUNK_H

#include "resource.h"
#include "Importer/OBJ.h"

#define TERRAIN_CHUNKS_LOD 3

#define TERRAIN_CHUNK_LOD0	100.0f
#define TERRAIN_CHUNK_LOD1	180.0f

class Tree
{
public:
	GLuint ID;
	std::string name;
};

class TerrainChunk
{
public:
	void Destroy();
	int  DrawGround(GLuint lod);
	void DrawGrass(GLuint lod, F32 d);
	void DrawTrees(GLuint lod, F32 d);
	int  DrawObjects(GLuint lod);
	void Load(U32 depth, ivec2 pos, ivec2 HMsize);

	inline vector<GLuint>&				getIndiceArray(GLuint lod)	{return m_tIndice[lod];}
	inline vector<GLuint>&				getGrassIndiceArray()		{return m_tGrassIndice;}
	inline vector<ImportedModel*>&		getObjectsArray()			{return m_tObject;}
	void									addObject(ImportedModel* obj);
	void									addTree(vec3 pos, F32 rotation, F32 scale);
	TerrainChunk() {}
	~TerrainChunk() {Destroy();}

private:
	void ComputeIndicesArray(U32 lod, U32 depth, ivec2 pos, ivec2 HMsize);
	void loadTrees();

private:
	vector<GLuint>		m_tIndice[TERRAIN_CHUNKS_LOD];
	U32					m_tIndOffsetW[TERRAIN_CHUNKS_LOD];
	U32					m_tIndOffsetH[TERRAIN_CHUNKS_LOD];

	vector<GLuint>	      m_tGrassIndice;
	vector<Tree>           m_tTrees;
	vector<Tree>::iterator m_tTreesIterator;

	vector<ImportedModel*>	m_tObject;
	string				    previousModel;
};

#endif


#ifndef QUADTREENODE
#define QUADTREENODE

#include "resource.h"

#include "BoundingBox.h"
#include "Terrain/TerrainChunk.h"

#define CHILD_NW	0
#define CHILD_NE	1
#define CHILD_SW	2
#define CHILD_SE	3

#define CHUNK_BIT_TESTCHILDREN		0x1
#define CHUNK_BIT_WATERREFLECTION	0x2



class Frustum;
class TerrainChunk;

class QuadtreeNode {
public:
	//fonction recursive de traitement des noeuds
	void Build(U32 depth, ivec2 pos, ivec2 HMsize, U32 minHMSize);
	void ComputeBoundingBox(const vec3* vertices);
	void Destroy();

	int  DrawGround(Frustum* pFrust, int options);
	void DrawGrass(bool drawInReflexion);
	int  DrawObjects(bool drawInReflexion);
	void DrawTrees(bool drawInReflexion);
	void DrawBBox(bool bTest);

	inline bool isALeaf() const							{return m_pChildren==0;}
	inline const BoundingBox&	getBoundingBox()		{return terrain_BBox;}
	inline void setBoundingBox(const BoundingBox& bbox)	{terrain_BBox = bbox;}
	inline QuadtreeNode*	getChildren()				{return m_pChildren;}
	inline TerrainChunk*	getChunk()					{return m_pTerrainChunk;}

	QuadtreeNode()  {m_pChildren = NULL; m_pTerrainChunk = NULL; m_nLOD = 0;}
	~QuadtreeNode() {Destroy();}

private:
	int				m_nLOD;				// Niveau de LOD
	F32			m_fDistance;		// Distance du centre par rapport � la cam�ra
	BoundingBox		terrain_BBox;				// Taille de la BBOX 3D englobant le noeud
	QuadtreeNode*	m_pChildren;		// Enfants du noeud
	TerrainChunk*	m_pTerrainChunk;	// Chunk (partie du terrain) si c'est une feuille
};

#endif

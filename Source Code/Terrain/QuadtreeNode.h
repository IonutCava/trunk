#ifndef QUADTREENODE
#define QUADTREENODE

#include "resource.h"
#include "Utility/Headers/BoundingBox.h"

#define CHILD_NW	0
#define CHILD_NE	1
#define CHILD_SW	2
#define CHILD_SE	3

#define CHUNK_BIT_TESTCHILDREN		0x1
#define CHUNK_BIT_WATERREFLECTION	0x2
#define CHUNK_BIT_DEPTHMAP			0x3


class Frustum;
class TerrainChunk;

class QuadtreeNode {
public:
	//fonction recursive de traitement des noeuds
	void Build(U32 depth, ivec2 pos, ivec2 HMsize, U32 minHMSize);
	void ComputeBoundingBox(const std::vector<vec3>& vertices);
	void Destroy();

	void DrawGround(I32 options);
	void DrawGrass(bool drawInReflexion);
	void DrawTrees(bool drawInReflexion);
	void DrawBBox();

	inline bool isALeaf() const							{return _children==0;}
	inline const BoundingBox&	getBoundingBox()		{return _terrainBBox;}
	inline void setBoundingBox(const BoundingBox& bbox)	{_terrainBBox = bbox;}
	inline QuadtreeNode*	getChildren()				{return _children;}
	inline TerrainChunk*	getChunk()					{return _terrainChunk;}

	QuadtreeNode()  {_children = NULL; _terrainChunk = NULL; _LOD = 0;}
	~QuadtreeNode() {Destroy();}

private:
	I32				_LOD;				// LOD level
	F32			    _camDistance;		// Distance to camera
	BoundingBox		_terrainBBox;		// Node BoundingBox
	QuadtreeNode*	_children;			// Node children
	TerrainChunk*	_terrainChunk;		// Terrain Chunk contained in node
};

#endif

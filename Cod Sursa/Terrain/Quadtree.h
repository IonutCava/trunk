#ifndef QUADTREE
#define QUADTREE

#include "resource.h"
#include "Utility/Headers/MathClasses.h"

class QuadtreeNode;
class BoundingBox;

class Quadtree {
public:
	void Build(BoundingBox* pBBox, ivec2 HMSize, U32 minHMSize);
	void ComputeBoundingBox(const vec3* vertices);
	void Destroy();

	int  DrawGround(bool drawInReflexion,bool drawDepthMap);
	void DrawGrass(bool drawInReflexion,bool drawDepthMap);
	void DrawTrees(bool drawInReflexion,bool drawDepthMap);
	void DrawBBox();

	QuadtreeNode*	FindLeaf(vec2& pos);

	Quadtree()	{m_pRoot = NULL;}
	~Quadtree()	{Destroy();}

private:
	QuadtreeNode*	m_pRoot;	// Noeud principal du Quadtree
};

#endif


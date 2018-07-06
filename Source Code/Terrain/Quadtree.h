#ifndef QUADTREE
#define QUADTREE

#include "resource.h"

class QuadtreeNode;
class BoundingBox;
class ivec2;
class Quadtree {
public:
	void Build(BoundingBox* pBBox, ivec2 HMSize, U32 minHMSize);
	void ComputeBoundingBox(const std::vector<vec3>& vertices);
	void Destroy();

	void DrawGround(bool drawInReflexion);
	void DrawGrass(bool drawInReflexion);
	void DrawTrees(bool drawInReflexion);
	void DrawBBox();

	QuadtreeNode*	FindLeaf(vec2& pos);

	Quadtree()	{_root = NULL;}
	~Quadtree()	{Destroy();}

private:
	QuadtreeNode*	_root;	
};

#endif


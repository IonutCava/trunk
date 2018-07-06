#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Headers/BoundingBox.h"
#include "Rendering/Frustum.h"
#include <assert.h>
 
using namespace std;

int Quadtree::DrawGround(bool drawInReflexion) {
	assert(m_pRoot);
	int options = CHUNK_BIT_TESTCHILDREN;
	if(drawInReflexion)	options |= CHUNK_BIT_WATERREFLECTION;
	return m_pRoot->DrawGround(&Frustum::getInstance(), options);
}

void Quadtree::DrawGrass(bool drawInReflexion)
{
	assert(m_pRoot);
	m_pRoot->DrawGrass(drawInReflexion);
}

int Quadtree::DrawObjects(bool drawInReflexion)
{
	assert(m_pRoot);
	return m_pRoot->DrawObjects(drawInReflexion);
}
void Quadtree::DrawTrees(bool drawInReflexion)
{
	assert(m_pRoot);
	m_pRoot->DrawTrees(drawInReflexion);
}

void Quadtree::DrawBBox() {
	assert(m_pRoot);
	m_pRoot->DrawBBox(true);
}


QuadtreeNode* Quadtree::FindLeaf(vec2& pos)
{
	assert(m_pRoot);
	QuadtreeNode* node = m_pRoot;

	while(!node->isALeaf()) {
		int i=0;
		for(i=0; i<4; i++) {
			QuadtreeNode* child = &(node->getChildren()[i]);
			if(child->getBoundingBox().ContainsPoint( vec3(pos.x, child->getBoundingBox().getCenter().y, pos.y) ))
			{
				node = child;
				break;
			}
		}

		if(i>=4) {
			return NULL;
		}
	}

	return node;
}


void Quadtree::Build(BoundingBox* pBBox,		
					 ivec2 HMsize,				
					 U32 minHMSize)	
{
	assert(pBBox);

	
	m_pRoot = New QuadtreeNode();
	m_pRoot->setBoundingBox(*pBBox);

	
	m_pRoot->Build(0, ivec2(0,0), HMsize, minHMSize);
}

void Quadtree::ComputeBoundingBox(const vec3* vertices)
{
	assert(m_pRoot);
	assert(vertices);
	m_pRoot->ComputeBoundingBox(vertices);

}


void Quadtree::Destroy()
{
	if(m_pRoot) {
		m_pRoot->Destroy();
		delete m_pRoot;
		m_pRoot = NULL;
	}
}



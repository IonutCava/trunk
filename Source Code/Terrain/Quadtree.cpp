#include "Quadtree.h"
#include "QuadtreeNode.h"
#include "Hardware/Video/GFXDevice.h"
 
void Quadtree::DrawGround(bool drawInReflexion) {
	assert(_root);
	int options = CHUNK_BIT_TESTCHILDREN;
	if(drawInReflexion)	options |= CHUNK_BIT_WATERREFLECTION;
	if(GFXDevice::getInstance().getDepthMapRendering()) options |= CHUNK_BIT_DEPTHMAP;
	_root->DrawGround(options);
	//_root->DrawBBox();
}

void Quadtree::DrawGrass(bool drawInReflexion)
{
	assert(_root);
	_root->DrawGrass(drawInReflexion);
}

void Quadtree::DrawTrees(bool drawInReflexion)
{
	assert(_root);
	_root->DrawTrees(drawInReflexion);
}

void Quadtree::DrawBBox() {
	assert(_root);
	_root->DrawBBox();
}


QuadtreeNode* Quadtree::FindLeaf(vec2& pos)
{
	assert(_root);
	QuadtreeNode* node = _root;

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

	
	_root = New QuadtreeNode();
	_root->setBoundingBox(*pBBox);

	
	_root->Build(0, ivec2(0,0), HMsize, minHMSize);
}

void Quadtree::ComputeBoundingBox(const std::vector<vec3>& vertices)
{
	assert(_root);
	assert(!vertices.empty());
	_root->ComputeBoundingBox(vertices);

}


void Quadtree::Destroy()
{
	if(_root) {
		delete _root;
		_root = NULL;
	}
}



#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Hardware/Video/GFXDevice.h"
 
void Quadtree::DrawGround(bool drawInReflection) {
	assert(_root);
	_root->DrawGround(drawInReflection);
}

void Quadtree::DrawGrass()
{
	assert(_root);
	_root->DrawGrass();
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


void Quadtree::Build(BoundingBox& terrainBBox,		
					 ivec2 HMsize,				
					 U32 minHMSize)	
{
	
	_root = New QuadtreeNode();
	_root->setBoundingBox(terrainBBox);
	_root->setParentShaderProgram(_parentShaderProgram);

	
	_root->Build(0, ivec2(0,0), HMsize, minHMSize);
}

BoundingBox& Quadtree::computeBoundingBox(const std::vector<vec3>& vertices){
	assert(_root);
	assert(!vertices.empty());
	 _root->computeBoundingBox(vertices);
	 return _root->getBoundingBox();;
}


void Quadtree::Destroy(){
	SAFE_DELETE(_root);
}



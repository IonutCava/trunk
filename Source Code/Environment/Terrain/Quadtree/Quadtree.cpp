#include "Headers/Quadtree.h"
#include "Headers/QuadtreeNode.h"
#include "Hardware/Video/Headers/GFXDevice.h"
 
void Quadtree::DrawGround(bool drawInReflection) {
	assert(_root);
    _root->DrawGround(drawInReflection ? CHUNK_BIT_WATERREFLECTION : 0, _parentVBO);
}

void Quadtree::DrawGrass(VertexBufferObject* const grassVBO)
{
	assert(_root);
	_root->DrawGrass(grassVBO);
}

void Quadtree::DrawBBox() {
	assert(_root);
	_root->DrawBBox();
}


QuadtreeNode* Quadtree::FindLeaf(const vec2<F32>& pos) {
	assert(_root);
	QuadtreeNode* node = _root;

	while(!node->isALeaf()) {
		int i=0;
		for(i=0; i<4; i++) {
			QuadtreeNode* child = &(node->getChildren()[i]);
			if(child->getBoundingBox().ContainsPoint( vec3<F32>(pos.x, child->getBoundingBox().getCenter().y, pos.y) ))
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
					 vec2<U32> HMsize,				
					 U32 minHMSize,
                     VertexBufferObject* const groundVBO)	
{
	
	_root = New QuadtreeNode();
	_root->setBoundingBox(terrainBBox);
	_root->setParentShaderProgram(_parentShaderProgram);

	
	_root->Build(0, vec2<U32>(0,0), HMsize, minHMSize,groundVBO);
}

BoundingBox& Quadtree::computeBoundingBox(const vectorImpl<vec3<F32> >& vertices){
	assert(_root);
	assert(!vertices.empty());
	 _root->computeBoundingBox(vertices);
	 return _root->getBoundingBox();;
}


void Quadtree::Destroy(){
	SAFE_DELETE(_root);
}



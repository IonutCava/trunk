#include "QuadtreeNode.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/BoundingBox.h"
#include "Terrain/TerrainChunk.h"
#include "Terrain/Terrain.h"
#include "Managers/SceneManager.h"
#include "Managers/CameraManager.h"

void QuadtreeNode::Build(U8 depth,		
						 ivec2 pos,					
						 ivec2 HMsize,				
						 U32 minHMSize)	
{
	_LOD = 0;

	U32 div = (U32)pow(2.0f, (F32)depth);
	ivec2 nodesize = HMsize/(div);
	if(nodesize.x%2==0) nodesize.x++;
	if(nodesize.y%2==0) nodesize.y++;
	ivec2 newsize = nodesize/2;


	
	if((U32)max(newsize.x, newsize.y) < minHMSize)
	{
		
		_terrainChunk = New TerrainChunk();
		_terrainChunk->Load(depth, pos, HMsize);

		
		_children = NULL;
		return;
	}

	//Cream 4 "copii"
	_children = new QuadtreeNode[4];

	// Calculam Bounding Box-ul "copiilor"
	vec3 center = _boundingBox.getCenter();
	_children[CHILD_NW].setBoundingBox( BoundingBox(_boundingBox.getMin(), center) );
	_children[CHILD_NE].setBoundingBox( BoundingBox(vec3(center.x, 0.0f, _boundingBox.getMin().z), vec3(_boundingBox.getMax().x, 0.0f, center.z)) );
	_children[CHILD_SW].setBoundingBox( BoundingBox(vec3(_boundingBox.getMin().x, 0.0f, center.z), vec3(center.x, 0.0f, _boundingBox.getMax().z)) );
	_children[CHILD_SE].setBoundingBox( BoundingBox(center, _boundingBox.getMax()) );

	// Calculam pozitia copiilor
	ivec2 tNewHMpos[4];
	tNewHMpos[CHILD_NW] = pos + ivec2(0, 0);
	tNewHMpos[CHILD_NE] = pos + ivec2(newsize.x, 0);
	tNewHMpos[CHILD_SW] = pos + ivec2(0, newsize.y);
	tNewHMpos[CHILD_SE] = pos + ivec2(newsize.x, newsize.y);


	
	for(int i=0; i<4; i++)
		_children[i].Build(depth+1, tNewHMpos[i], HMsize, minHMSize);
}

bool QuadtreeNode::computeBoundingBox(const std::vector<vec3>& vertices){
	SceneGraph* sceneGraph = SceneManager::getInstance().getActiveScene()->getSceneGraph();
	assert(!vertices.empty());

	if(_terrainChunk != NULL) {
		std::vector<U32>& tIndices = _terrainChunk->getIndiceArray(0);
		F32 tempMin = 100000.0f;
		F32 tempMax = -100000.0f;
		
		for(U32 i=0; i<tIndices.size(); i++) {
			F32 y = vertices[ tIndices[i] ].y;

			if(y > tempMax) tempMax = y;
			if(y < tempMin) tempMin = y;
		}

		_boundingBox.setMin(vec3(_boundingBox.getMin().x, tempMin,_boundingBox.getMin().z));
		_boundingBox.setMax(vec3(_boundingBox.getMax().x, tempMax,_boundingBox.getMax().z));

	}


	if(_children != NULL) {
		for(int i=0; i<4; i++) {
			_children[i].computeBoundingBox(vertices);

			if(_boundingBox.getMin().y > _children[i]._boundingBox.getMin().y)	
				_boundingBox.setMin(vec3(_boundingBox.getMin().x,_children[i]._boundingBox.getMin().y,_boundingBox.getMin().z));
			if(_boundingBox.getMax().y < _children[i]._boundingBox.getMax().y)	
				_boundingBox.setMax(vec3(_boundingBox.getMax().x,_children[i]._boundingBox.getMax().y,_boundingBox.getMax().z));
		}
	}
	_boundingBox.isComputed() = true;
	return true;
}


void QuadtreeNode::Destroy(){
	if(_children != NULL) {
		delete [] _children;
		_children = NULL;
	}
	if(_terrainChunk != NULL){
		delete _terrainChunk;
		_terrainChunk = NULL;
	}
}
//ToDo: Change vegetation rendering and generation! -Ionut
void QuadtreeNode::DrawGrass(){
	if(!_children) {
		assert(_terrainChunk);
		if( _LOD>=0 ){
			_terrainChunk->DrawGrass(_LOD, _camDistance);
		}else
			return;
	}
	else {
		if( _LOD>=0 )
			for(I8 i=0; i<4; i++)
				_children[i].DrawGrass();
		return;		
	}
}

void QuadtreeNode::DrawBBox()
{
	if(!_children) {
		assert(_terrainChunk);
			GFXDevice::getInstance().drawBox3D(_boundingBox.getMin(),_boundingBox.getMax());
	}
	else {
		if( _LOD>=0 )
			for(int i=0; i<4; i++)
				_children[i].DrawBBox();
	}
}


void QuadtreeNode::DrawGround(I32 options)
{
	_LOD = -1;
	Frustum& pFrust = Frustum::getInstance();
	vec3 center = _boundingBox.getCenter();				
	F32 radius = (_boundingBox.getMax()-center).length();	

	if(options & CHUNK_BIT_TESTCHILDREN) {
		if(!_boundingBox.ContainsPoint(pFrust.getEyePos()))
		{
			I8 resSphereInFrustum = pFrust.ContainsSphere(center, radius);
			switch(resSphereInFrustum) {
				case FRUSTUM_OUT: return;		
				case FRUSTUM_IN:
					options &= ~CHUNK_BIT_TESTCHILDREN;				
					break;		
				case FRUSTUM_INTERSECT:								
				{		
					I8 resBoxInFrustum = pFrust.ContainsBoundingBox(_boundingBox);
					switch(resBoxInFrustum) {
						case FRUSTUM_IN: options &= ~CHUNK_BIT_TESTCHILDREN; break;
						case FRUSTUM_OUT: return;
					}
				}
			}
		}
	}

	_LOD = 0; 

	if(!_children) {
		assert(_terrainChunk);

		if(options & CHUNK_BIT_WATERREFLECTION) {
			_terrainChunk->DrawGround(TERRAIN_CHUNKS_LOD-1,true);
		}
		else {
			
			vec3 vEyeToChunk = this->getBoundingBox().getCenter() - pFrust.getEyePos();
			_camDistance = vEyeToChunk.length();
			I8 lod = 0;
			if(_camDistance > TERRAIN_CHUNK_LOD1)		lod = 2;
			else if(_camDistance > TERRAIN_CHUNK_LOD0)	lod = 1;
			_LOD = lod;

			_terrainChunk->DrawGround(lod);
		}
	}
	else {
		for(int i=0; i<4; i++)
			_children[i].DrawGround(options);
	}

}



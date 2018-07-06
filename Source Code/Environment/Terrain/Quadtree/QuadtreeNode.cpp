#include "Headers/QuadtreeNode.h"
#include "Rendering/Headers/Frustum.h"
#include "Utility/Headers/BoundingBox.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"

void QuadtreeNode::Build(U8 depth,		
						 vec2<U32> pos,					
						 vec2<U32> HMsize,				
						 U32 minHMSize)	
{
	_LOD = 0;

	U32 div = (U32)pow(2.0f, (F32)depth);
	vec2<U32> nodesize = HMsize/(div);
	if(nodesize.x%2==0) nodesize.x++;
	if(nodesize.y%2==0) nodesize.y++;
	vec2<U32> newsize = nodesize/2;


	
	if((U32)max(newsize.x, newsize.y) < minHMSize)	{
		
		_terrainChunk = New TerrainChunk();
		_terrainChunk->Load(depth, pos, HMsize);

		_children = NULL;
		return;
	}

	// Create 4 children
	_children = New QuadtreeNode[4];

	// Compute children bounding boxes
	vec3<F32> center = _boundingBox.getCenter();
	_children[CHILD_NW].setBoundingBox( BoundingBox(_boundingBox.getMin(), center) );
	_children[CHILD_NE].setBoundingBox( BoundingBox(vec3<F32>(center.x, 0.0f, _boundingBox.getMin().z), vec3<F32>(_boundingBox.getMax().x, 0.0f, center.z)) );
	_children[CHILD_SW].setBoundingBox( BoundingBox(vec3<F32>(_boundingBox.getMin().x, 0.0f, center.z), vec3<F32>(center.x, 0.0f, _boundingBox.getMax().z)) );
	_children[CHILD_SE].setBoundingBox( BoundingBox(center, _boundingBox.getMax()) );

	// Compute children positions
	vec2<U32> tNewHMpos[4];
	tNewHMpos[CHILD_NW] = pos + vec2<U32>(0, 0);
	tNewHMpos[CHILD_NE] = pos + vec2<U32>(newsize.x, 0);
	tNewHMpos[CHILD_SW] = pos + vec2<U32>(0, newsize.y);
	tNewHMpos[CHILD_SE] = pos + vec2<U32>(newsize.x, newsize.y);


	
	for(I8 i=0; i<4; i++){
		_children[i].setParentShaderProgram(_parentShaderProgram);
		_children[i].Build(depth+1, tNewHMpos[i], HMsize, minHMSize);
	}
}

bool QuadtreeNode::computeBoundingBox(const std::vector<vec3<F32> >& vertices){
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

		_boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x, tempMin,_boundingBox.getMin().z));
		_boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x, tempMax,_boundingBox.getMax().z));

	}


	if(_children != NULL) {
		for(I8 i=0; i<4; i++) {
			_children[i].computeBoundingBox(vertices);

			if(_boundingBox.getMin().y > _children[i]._boundingBox.getMin().y)	
				_boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x,_children[i]._boundingBox.getMin().y,_boundingBox.getMin().z));
			if(_boundingBox.getMax().y < _children[i]._boundingBox.getMax().y)	
				_boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x,_children[i]._boundingBox.getMax().y,_boundingBox.getMax().z));
		}
	}
	_boundingBox.isComputed() = true;
	return true;
}


void QuadtreeNode::Destroy(){
	SAFE_DELETE_ARRAY(_children);
	SAFE_DELETE(_terrainChunk);
}
///ToDo: Change vegetation rendering and generation! -Ionut
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

void QuadtreeNode::DrawBBox(){
	if(!_children) {
		assert(_terrainChunk);
			GFX_DEVICE.drawBox3D(_boundingBox.getMin(),_boundingBox.getMax());
	}else {
		if( _LOD>=0 )
			for(I8 i=0; i<4; i++)
				_children[i].DrawBBox();
	}
}


void QuadtreeNode::DrawGround(I32 options)
{
	_LOD = -1;
	Frustum& pFrust = Frustum::getInstance();
	vec3<F32> center = _boundingBox.getCenter();				
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
			
			vec3<F32> vEyeToChunk = this->getBoundingBox().getCenter() - pFrust.getEyePos();
			_camDistance = vEyeToChunk.length();
			I8 lod = 0;
			if(_camDistance > TERRAIN_CHUNK_LOD1)		lod = 2;
			else if(_camDistance > TERRAIN_CHUNK_LOD0)	lod = 1;
			_LOD = lod;
			getParentShaderProgram()->Uniform("LOD", (I32)lod);
			_terrainChunk->DrawGround(lod);
		}
	}
	else {
		for(I8 i=0; i<4; i++)
			_children[i].DrawGround(options);
	}

}



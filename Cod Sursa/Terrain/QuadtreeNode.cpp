#include "QuadtreeNode.h"
#include "Rendering/Frustum.h"
#include "Utility/Headers/BoundingBox.h"
#include "Terrain/TerrainChunk.h"

void QuadtreeNode::Build(U32 depth,		
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
		
		_terrainChunk = new TerrainChunk();
		_terrainChunk->Load(depth, pos, HMsize);

		
		_children = NULL;
		return;
	}

	//Cream 4 "copii"
	_children = new QuadtreeNode[4];

	// Calculam Bounding Box-ul "copiilor"
	vec3 center = _terrainBBox.getCenter();
	_children[CHILD_NW].setBoundingBox( BoundingBox(_terrainBBox.getMin(), center) );
	_children[CHILD_NE].setBoundingBox( BoundingBox(vec3(center.x, 0.0f, _terrainBBox.getMin().z), vec3(_terrainBBox.getMax().x, 0.0f, center.z)) );
	_children[CHILD_SW].setBoundingBox( BoundingBox(vec3(_terrainBBox.getMin().x, 0.0f, center.z), vec3(center.x, 0.0f, _terrainBBox.getMax().z)) );
	_children[CHILD_SE].setBoundingBox( BoundingBox(center, _terrainBBox.getMax()) );

	// Calculam pozitia copiilor
	ivec2 tNewHMpos[4];
	tNewHMpos[CHILD_NW] = pos + ivec2(0, 0);
	tNewHMpos[CHILD_NE] = pos + ivec2(newsize.x, 0);
	tNewHMpos[CHILD_SW] = pos + ivec2(0, newsize.y);
	tNewHMpos[CHILD_SE] = pos + ivec2(newsize.x, newsize.y);


	
	for(int i=0; i<4; i++)
		_children[i].Build(depth+1, tNewHMpos[i], HMsize, minHMSize);
}

void QuadtreeNode::ComputeBoundingBox(const std::vector<vec3>& vertices)
{
	assert(!vertices.empty());

	if(_terrainChunk) {
		std::vector<U32>& tIndices = _terrainChunk->getIndiceArray(0);
		F32 tempMin = 100000.0f;
		F32 tempMax = -100000.0f;
		
		for(U32 i=0; i<tIndices.size(); i++) {
			F32 y = vertices[ tIndices[i] ].y;

			if(y > tempMax) tempMax = y;
			if(y < tempMin) tempMin = y;
		}

		_terrainBBox.setMin(vec3(_terrainBBox.getMin().x, tempMin,_terrainBBox.getMin().z));
		_terrainBBox.setMax(vec3(_terrainBBox.getMax().x, tempMax,_terrainBBox.getMax().z));

		for(U16 i = 0; i < _terrainChunk->getTreeArray().size(); i++){
			_terrainBBox.Add(_terrainChunk->getTreeArray()[i]->getBoundingBox());
		}
	}


	if(_children) {
		for(int i=0; i<4; i++) {
			_children[i].ComputeBoundingBox(vertices);

			if(_terrainBBox.getMin().y > _children[i]._terrainBBox.getMin().y)	
				_terrainBBox.setMin(vec3(_terrainBBox.getMin().x,_children[i]._terrainBBox.getMin().y,_terrainBBox.getMin().z));
			if(_terrainBBox.getMax().y < _children[i]._terrainBBox.getMax().y)	
				_terrainBBox.setMax(vec3(_terrainBBox.getMax().x,_children[i]._terrainBBox.getMax().y,_terrainBBox.getMax().z));
		}
	}
	_terrainBBox.isComputed() = true;
}


void QuadtreeNode::Destroy()
{
	if(_children) {
		delete [] _children;
		_children = NULL;
	}
	if(_terrainChunk)
		delete _terrainChunk;
}

void QuadtreeNode::DrawGrass(bool drawInReflexion)
{
	if(!_children) {
		assert(_terrainChunk);
		if( _LOD>=0 )
			_terrainChunk->DrawGrass( (U32)_LOD, _camDistance );
		else
			return;
	}
	else {
		if( _LOD>=0 )
			for(int i=0; i<4; i++)
				_children[i].DrawGrass(drawInReflexion);
		return;		
	}
}

void QuadtreeNode::DrawTrees(bool drawInReflexion)
{
	if(!_children) {
		assert(_terrainChunk);
		if( _LOD>=0 )
			_terrainChunk->DrawTrees(drawInReflexion ? TERRAIN_CHUNKS_LOD-1 : (U8)_LOD , _camDistance );
		else
			return;
	}
	else {
		if( _LOD>=0 )
			for(int i=0; i<4; i++)
				_children[i].DrawTrees(drawInReflexion);
		return;		
	}
}

void QuadtreeNode::DrawBBox()
{
	if(!_children) {
		assert(_terrainChunk);
			GFXDevice::getInstance().drawBox3D(_terrainBBox.getMin(),_terrainBBox.getMax());
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
	vec3 center = _terrainBBox.getCenter();				
	F32 radius = (_terrainBBox.getMax()-center).length();	

	if(options & CHUNK_BIT_TESTCHILDREN) {
		if(!_terrainBBox.ContainsPoint(pFrust.getEyePos()))
		{
			I8 resSphereInFrustum = pFrust.ContainsSphere(center, radius);
			switch(resSphereInFrustum) {
				case FRUSTUM_OUT: return;		
				case FRUSTUM_IN:
					options &= ~CHUNK_BIT_TESTCHILDREN;				
					break;		
				case FRUSTUM_INTERSECT:								
				{		
					I8 resBoxInFrustum = pFrust.ContainsBoundingBox(_terrainBBox);
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
			_terrainChunk->DrawGround(TERRAIN_CHUNKS_LOD-1);
		}
		else {
			
			vec3 vEyeToChunk = this->getBoundingBox().getCenter() - pFrust.getEyePos();
			_camDistance = vEyeToChunk.length();
			U32 lod = 0;
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



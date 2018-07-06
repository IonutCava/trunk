#include "Headers/QuadtreeNode.h"

#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

void QuadtreeNode::Build(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, U32 minHMSize, VertexBufferObject* const groundVBO, U32& chunkCount){
    _LOD = 0;

    U32 div = (U32)pow(2.0f, (F32)depth);
    vec2<U32> nodesize = HMsize/(div);
    if(nodesize.x%2==0) nodesize.x++;
    if(nodesize.y%2==0) nodesize.y++;
    vec2<U32> newsize = nodesize/2;

    if(std::max(newsize.x, newsize.y) < minHMSize)	{
        _terrainChunk = New TerrainChunk();
        _terrainChunk->Load(depth, pos, HMsize, groundVBO);
		chunkCount++;
        _children = NULL;
        return;
    }

    // Create 4 children
    _children = New QuadtreeNode[4];

    // Compute children bounding boxes
    const vec3<F32>& center = _boundingBox.getCenter();
    _children[CHILD_NW].setBoundingBox( BoundingBox(_boundingBox.getMin(), center) );
    _children[CHILD_NE].setBoundingBox( BoundingBox(vec3<F32>(center.x, 0.0f, _boundingBox.getMin().z), vec3<F32>(_boundingBox.getMax().x, 0.0f, center.z)) );
    _children[CHILD_SW].setBoundingBox( BoundingBox(vec3<F32>(_boundingBox.getMin().x, 0.0f, center.z), vec3<F32>(center.x, 0.0f, _boundingBox.getMax().z)) );
    _children[CHILD_SE].setBoundingBox( BoundingBox(center, _boundingBox.getMax()) );
    _children[CHILD_NW].setParentShaderProgram(_parentShaderProgram);
    _children[CHILD_NE].setParentShaderProgram(_parentShaderProgram);
    _children[CHILD_SW].setParentShaderProgram(_parentShaderProgram);
    _children[CHILD_SE].setParentShaderProgram(_parentShaderProgram);
    // Compute children positions
    vec2<U32> tNewHMpos[4];
    tNewHMpos[CHILD_NW] = pos + vec2<U32>(0, 0);
    tNewHMpos[CHILD_NE] = pos + vec2<U32>(newsize.x, 0);
    tNewHMpos[CHILD_SW] = pos + vec2<U32>(0, newsize.y);
    tNewHMpos[CHILD_SE] = pos + vec2<U32>(newsize.x, newsize.y);
    _children[CHILD_NW].Build(depth+1, tNewHMpos[CHILD_NW], HMsize, minHMSize, groundVBO, chunkCount);
    _children[CHILD_NE].Build(depth+1, tNewHMpos[CHILD_NE], HMsize, minHMSize, groundVBO, chunkCount);
    _children[CHILD_SW].Build(depth+1, tNewHMpos[CHILD_SW], HMsize, minHMSize, groundVBO, chunkCount);
    _children[CHILD_SE].Build(depth+1, tNewHMpos[CHILD_SE], HMsize, minHMSize, groundVBO, chunkCount);
}

bool QuadtreeNode::computeBoundingBox(const vectorImpl<vec3<F32> >& vertices){
    assert(!vertices.empty());

    if(_terrainChunk != NULL) {
        _boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x, _terrainChunk->getMinHeight(),_boundingBox.getMin().z));
        _boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x, _terrainChunk->getMaxHeight(),_boundingBox.getMax().z));
    }

    if(_children != NULL) {
        for(I8 i=0; i<4; i++) {
            _children[i].computeBoundingBox(vertices);

            if(_boundingBox.getMin().y > _children[i]._boundingBox.getMin().y){
                _boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x,_children[i]._boundingBox.getMin().y,_boundingBox.getMin().z));
            }

            if(_boundingBox.getMax().y < _children[i]._boundingBox.getMax().y){
                _boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x,_children[i]._boundingBox.getMax().y,_boundingBox.getMax().z));
            }
        }
    }
    _boundingBox.setComputed(true);
    return true;
}

void QuadtreeNode::Destroy(){
    SAFE_DELETE_ARRAY(_children);
    SAFE_DELETE(_terrainChunk);
}

void QuadtreeNode::GenerateGrassIndexBuffer(U32 bilboardCount){
    if(!_children){
        assert(_terrainChunk);
        _terrainChunk->GenerateGrassIndexBuffer(bilboardCount);
    }else{
        _children[CHILD_NW].GenerateGrassIndexBuffer(bilboardCount);
        _children[CHILD_NE].GenerateGrassIndexBuffer(bilboardCount);
        _children[CHILD_SW].GenerateGrassIndexBuffer(bilboardCount);
        _children[CHILD_SE].GenerateGrassIndexBuffer(bilboardCount);
    }
}

#pragma message("ToDo: Change vegetation rendering and generation system. Stop relying on terrain! -Ionut")
void QuadtreeNode::DrawGrass(U32 geometryIndex, Transform* const parentTransform){
    if(_LOD < 0  || !isInView(CHUNK_BIT_TESTCHILDREN)) return;

    if(!_children){
        assert(_terrainChunk);
         _terrainChunk->DrawGrass(_LOD, _camDistance, geometryIndex, parentTransform);
    }else{
        _children[CHILD_NW].DrawGrass(geometryIndex, parentTransform);
        _children[CHILD_NE].DrawGrass(geometryIndex, parentTransform);
        _children[CHILD_SW].DrawGrass(geometryIndex, parentTransform);
        _children[CHILD_SE].DrawGrass(geometryIndex, parentTransform);
    }
}

void QuadtreeNode::DrawBBox(){
    if(!_children) {
        assert(_terrainChunk);
        GFX_DEVICE.drawBox3D(_boundingBox.getMin(),_boundingBox.getMax(),mat4<F32>());
    }else {
        if( _LOD < 0 ) return;
        _children[CHILD_NW].DrawBBox();
        _children[CHILD_NE].DrawBBox();
        _children[CHILD_SW].DrawBBox();
        _children[CHILD_SE].DrawBBox();
    }
}

bool QuadtreeNode::isInView(I32 options) {
	const Frustum& frust = Frustum::getInstance();
    const vec3<F32>& center = _boundingBox.getCenter();
    const vec3<F32>& eyePos = frust.getEyePos();
	   
    _camDistance = vec3<F32>(center - eyePos).length();
	_LOD = 0;
	if(_camDistance > Config::TERRAIN_CHUNK_LOD1)		_LOD = 2;
    else if(_camDistance > Config::TERRAIN_CHUNK_LOD0)	_LOD = 1;

    F32 radius = (_boundingBox.getMax()-center).length();
	if(options & CHUNK_BIT_TESTCHILDREN) {
        if(!_boundingBox.ContainsPoint(eyePos))	{
            switch(frust.ContainsSphere(center, radius)) {
                case FRUSTUM_OUT:	return false;
                case FRUSTUM_IN:	options &= ~CHUNK_BIT_TESTCHILDREN;	break;
                case FRUSTUM_INTERSECT:	{
                    switch(frust.ContainsBoundingBox(_boundingBox)) {
                        case FRUSTUM_IN: options &= ~CHUNK_BIT_TESTCHILDREN; break;
                        case FRUSTUM_OUT: return false;
                    };//inner switch
                };//case FRUSTUM_INTERSECT
            };//outer case
        }//if
    }//CHUNK_BIT_TESTCHILDREN option

	return true;
}

void QuadtreeNode::DrawGround(I32 options,VertexBufferObject* const terrainVBO){
  
	if(!isInView(options))
		return;

    if(!_children) {
        assert(_terrainChunk);

        if(options & CHUNK_BIT_WATERREFLECTION)
            _terrainChunk->DrawGround(Config::TERRAIN_CHUNKS_LOD-1,_parentShaderProgram,terrainVBO);
		else
			_terrainChunk->DrawGround(_LOD,_parentShaderProgram,terrainVBO);
    }else{
        _children[CHILD_NW].DrawGround(options, terrainVBO);
        _children[CHILD_NE].DrawGround(options, terrainVBO);
        _children[CHILD_SW].DrawGround(options, terrainVBO);
        _children[CHILD_SE].DrawGround(options, terrainVBO);
    }
}
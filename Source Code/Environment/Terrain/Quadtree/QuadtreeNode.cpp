#include "Headers/QuadtreeNode.h"

#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

QuadtreeNode::QuadtreeNode()
{
    _children[CHILD_NW] = nullptr;
    _children[CHILD_NE] = nullptr;
    _children[CHILD_SW] = nullptr;
    _children[CHILD_SE] = nullptr;
    _terrainChunk = nullptr;
    _LOD = 0;
    _terLoDOffset = 0.0f;
    _minHMSize = 0;
}

QuadtreeNode::~QuadtreeNode()
{
    MemoryManager::SAFE_DELETE( _children[CHILD_NW] );
    MemoryManager::SAFE_DELETE( _children[CHILD_NE] );
    MemoryManager::SAFE_DELETE( _children[CHILD_SW] );
    MemoryManager::SAFE_DELETE( _children[CHILD_SE] );
    MemoryManager::SAFE_DELETE( _terrainChunk );
}

void QuadtreeNode::Build( U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, U32 minHMSize, Terrain* const terrain, U32& chunkCount ) {
    _LOD = 0;
    _minHMSize = minHMSize;
    U32 div = (U32)std::pow(2.0f, (F32)depth);
    vec2<U32> nodesize = HMsize/(div);
    if(nodesize.x%2==0) nodesize.x++;
    if(nodesize.y%2==0) nodesize.y++;
    vec2<U32> newsize = nodesize/2;

    _terLoDOffset = (_minHMSize * 5.0f) / 100.0f;

    if (std::max(newsize.x, newsize.y) < _minHMSize)	{
        _terrainChunk = New TerrainChunk(terrain, this);
        _terrainChunk->Load(depth, pos, _minHMSize, HMsize, terrain);
		chunkCount++;
        return;
    }

    // Create 4 children
    _children[CHILD_NW] = New QuadtreeNode();
    _children[CHILD_NE] = New QuadtreeNode();
    _children[CHILD_SW] = New QuadtreeNode();
    _children[CHILD_SE] = New QuadtreeNode();

    // Compute children bounding boxes
    const vec3<F32>& center = _boundingBox.getCenter();
    _children[CHILD_NW]->setBoundingBox(BoundingBox(_boundingBox.getMin(), center));
    _children[CHILD_NE]->setBoundingBox(BoundingBox(vec3<F32>(center.x, 0.0f, _boundingBox.getMin().z), vec3<F32>(_boundingBox.getMax().x, 0.0f, center.z)));
    _children[CHILD_SW]->setBoundingBox(BoundingBox(vec3<F32>(_boundingBox.getMin().x, 0.0f, center.z), vec3<F32>(center.x, 0.0f, _boundingBox.getMax().z)));
    _children[CHILD_SE]->setBoundingBox(BoundingBox(center, _boundingBox.getMax()));
    // Compute children positions
    vec2<U32> tNewHMpos[4];
    tNewHMpos[CHILD_NW] = pos + vec2<U32>(0, 0);
    tNewHMpos[CHILD_NE] = pos + vec2<U32>(newsize.x, 0);
    tNewHMpos[CHILD_SW] = pos + vec2<U32>(0, newsize.y);
    tNewHMpos[CHILD_SE] = pos + vec2<U32>(newsize.x, newsize.y);
    _children[CHILD_NW]->Build( depth + 1, tNewHMpos[CHILD_NW], HMsize, _minHMSize, terrain, chunkCount );
    _children[CHILD_NE]->Build( depth + 1, tNewHMpos[CHILD_NE], HMsize, _minHMSize, terrain, chunkCount );
    _children[CHILD_SW]->Build( depth + 1, tNewHMpos[CHILD_SW], HMsize, _minHMSize, terrain, chunkCount );
    _children[CHILD_SE]->Build( depth + 1, tNewHMpos[CHILD_SE], HMsize, _minHMSize, terrain, chunkCount );
}

bool QuadtreeNode::computeBoundingBox(){
    if(_terrainChunk != nullptr) {
        _boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x, _terrainChunk->getMinHeight(),_boundingBox.getMin().z));
        _boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x, _terrainChunk->getMaxHeight(),_boundingBox.getMax().z));
    }

    if (!isALeaf()) {
        for(I8 i = 0; i < 4; i++) {
            _children[i]->computeBoundingBox();

            if (_boundingBox.getMin().y > _children[i]->_boundingBox.getMin().y){
                _boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x, _children[i]->_boundingBox.getMin().y, _boundingBox.getMin().z));
            }

            if (_boundingBox.getMax().y < _children[i]->_boundingBox.getMax().y){
                _boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x, _children[i]->_boundingBox.getMax().y, _boundingBox.getMax().z));
            }
        }
    }

    _boundingBox.setComputed(true);
    _boundingSphere.fromBoundingBox(_boundingBox);
    return true;
}

void QuadtreeNode::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState) {
    
    F32 camDistance = _boundingSphere.getCenter().distance(sceneState.getRenderState().getCameraConst().getEye()) - _terLoDOffset;
    F32 sphereRadius = _boundingSphere.getRadius();
    _LOD = camDistance >= sphereRadius ? (camDistance >= (sphereRadius * 2) ? 2 : 1) : 0;

    if (!isALeaf()) {
        _children[CHILD_NW]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_NE]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_SW]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_SE]->sceneUpdate(deltaTime, sgn, sceneState);
    }
}

bool QuadtreeNode::isInView(U32 options, const SceneRenderState& sceneRenderState) const {
	if(bitCompare(options, CHUNK_BIT_TESTCHILDREN)) {
        const Camera& cam = sceneRenderState.getCameraConst();
        if (!bitCompare(options, CHUNK_BIT_SHADOWMAP)) {
            const vec3<F32>& eye = cam.getEye();
            F32 visibilityDistance = GET_ACTIVE_SCENE()->state().getGeneralVisibility() + _boundingSphere.getRadius();
            if (_boundingSphere.getCenter().distance(eye) > visibilityDistance){
                if (_boundingBox.nearestDistanceFromPointSquared(eye) - _terLoDOffset > std::min(visibilityDistance, sceneRenderState.getCameraConst().getZPlanes().y))
                    return false;
            }
        }
        if (!_boundingBox.ContainsPoint(cam.getEye()))	{
            const Frustum& frust = cam.getFrustumConst();
            switch (frust.ContainsSphere(_boundingSphere.getCenter(), _boundingSphere.getRadius())) {
                case Frustum::FRUSTUM_OUT:	return false;
                case Frustum::FRUSTUM_IN:	options &= ~CHUNK_BIT_TESTCHILDREN;	break;
                case Frustum::FRUSTUM_INTERSECT:	{
                    switch (frust.ContainsBoundingBox(_boundingBox)) {
                        case Frustum::FRUSTUM_IN: options &= ~CHUNK_BIT_TESTCHILDREN; break;
                        case Frustum::FRUSTUM_OUT: return false;
                    };//inner switch
                };//case FRUSTUM_INTERSECT
            };//outer case
        }//if
    }//CHUNK_BIT_TESTCHILDREN option
    
	return true;
}


void QuadtreeNode::drawBBox() const {
    GFX_DEVICE.drawBox3D(_boundingBox.getMin(), _boundingBox.getMax(), vec4<U8>(0, 128, 255, 255));

    if (!isALeaf()){
        _children[CHILD_NW]->drawBBox();
        _children[CHILD_NE]->drawBBox();
        _children[CHILD_SW]->drawBBox();
        _children[CHILD_SE]->drawBBox();
    }
}

void QuadtreeNode::createDrawCommand(U32 options, const SceneRenderState& sceneRenderState, vectorImpl<GenericDrawCommand>& drawCommandsOut) {

    if (!isInView(options, sceneRenderState)) {
        return;
    }

    if (isALeaf()) {
        assert(_terrainChunk);
        _terrainChunk->createDrawCommand(bitCompare(options, CHUNK_BIT_WATERREFLECTION) ? Config::TERRAIN_CHUNKS_LOD - 1 : _LOD, drawCommandsOut);
    }else{
        _children[CHILD_NW]->createDrawCommand(options, sceneRenderState, drawCommandsOut);
        _children[CHILD_NE]->createDrawCommand(options, sceneRenderState, drawCommandsOut);
        _children[CHILD_SW]->createDrawCommand(options, sceneRenderState, drawCommandsOut);
        _children[CHILD_SE]->createDrawCommand(options, sceneRenderState, drawCommandsOut);
    }
}

};
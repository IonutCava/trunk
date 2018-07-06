#include "Headers/QuadtreeNode.h"

#include "Managers/Headers/SceneManager.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

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
    SAFE_DELETE(_children[CHILD_NW]);
    SAFE_DELETE(_children[CHILD_NE]);
    SAFE_DELETE(_children[CHILD_SW]);
    SAFE_DELETE(_children[CHILD_SE]);
    SAFE_DELETE(_terrainChunk);
}

void QuadtreeNode::Build(U8 depth, const vec2<U32>& pos, const vec2<U32>& HMsize, U32 minHMSize, VertexBuffer* const groundVB, Terrain* const parentTerrain, U32& chunkCount){
    _LOD = 0;
    _minHMSize = minHMSize;
    U32 div = (U32)pow(2.0f, (F32)depth);
    vec2<U32> nodesize = HMsize/(div);
    if(nodesize.x%2==0) nodesize.x++;
    if(nodesize.y%2==0) nodesize.y++;
    vec2<U32> newsize = nodesize/2;

    if (std::max(newsize.x, newsize.y) < _minHMSize)	{
        _terrainChunk = New TerrainChunk(groundVB, parentTerrain);
        _terrainChunk->Load(depth, pos, HMsize);
		chunkCount++;
        return;
    }

    _terLoDOffset = (_minHMSize * 5.0f) / 100.0f;

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
    _children[CHILD_NW]->Build(depth + 1, tNewHMpos[CHILD_NW], HMsize, _minHMSize, groundVB, parentTerrain, chunkCount);
    _children[CHILD_NE]->Build(depth + 1, tNewHMpos[CHILD_NE], HMsize, _minHMSize, groundVB, parentTerrain, chunkCount);
    _children[CHILD_SW]->Build(depth + 1, tNewHMpos[CHILD_SW], HMsize, _minHMSize, groundVB, parentTerrain, chunkCount);
    _children[CHILD_SE]->Build(depth + 1, tNewHMpos[CHILD_SE], HMsize, _minHMSize, groundVB, parentTerrain, chunkCount);
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
    _LOD = camDistance > _boundingSphere.getRadius() ? (camDistance > _boundingSphere.getDiameter() ? 2 : 1) : 0;

    if (!isALeaf()) {
        _children[CHILD_NW]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_NE]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_SW]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[CHILD_SE]->sceneUpdate(deltaTime, sgn, sceneState);
    }
}

bool QuadtreeNode::isInView(U32 options, const SceneRenderState& sceneRenderState) const {
	if(options & CHUNK_BIT_TESTCHILDREN) {
        const Camera& cam = sceneRenderState.getCameraConst();
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


void QuadtreeNode::DrawBBox() const {
    GFX_DEVICE.drawBox3D(_boundingBox.getMin(), _boundingBox.getMax(), mat4<F32>());

    if (!isALeaf()){
        _children[CHILD_NW]->DrawBBox();
        _children[CHILD_NE]->DrawBBox();
        _children[CHILD_SW]->DrawBBox();
        _children[CHILD_SE]->DrawBBox();
    }
}

void QuadtreeNode::DrawGround(U32 options, const SceneRenderState& sceneRenderState) const {

    if (!isInView(options, sceneRenderState))
        return;

    if (isALeaf()) {
        assert(_terrainChunk);
        _terrainChunk->DrawGround(options & CHUNK_BIT_WATERREFLECTION ? Config::TERRAIN_CHUNKS_LOD - 1 : _LOD);
    }else{
        _children[CHILD_NW]->DrawGround(options, sceneRenderState);
        _children[CHILD_NE]->DrawGround(options, sceneRenderState);
        _children[CHILD_SW]->DrawGround(options, sceneRenderState);
        _children[CHILD_SE]->DrawGround(options, sceneRenderState);
    }
}

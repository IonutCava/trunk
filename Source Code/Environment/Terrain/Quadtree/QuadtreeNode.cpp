#include "Headers/QuadtreeNode.h"

#include "Managers/Headers/SceneManager.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/IMPrimitive.h"
#include "Platform/Video/Headers/RenderStateBlock.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainChunk.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

QuadtreeNode::QuadtreeNode()
    : _terrainChunk(nullptr),
      _bbPrimitive(nullptr)
{
    _children[to_const_uint(ChildPosition::CHILD_NW)] = nullptr;
    _children[to_const_uint(ChildPosition::CHILD_NE)] = nullptr;
    _children[to_const_uint(ChildPosition::CHILD_SW)] = nullptr;
    _children[to_const_uint(ChildPosition::CHILD_SE)] = nullptr;

    _terLoDOffset = 0.0f;
    _minHMSize = 0;
}

QuadtreeNode::~QuadtreeNode()
{
    MemoryManager::DELETE(_terrainChunk);
    MemoryManager::DELETE(_children[to_const_uint(ChildPosition::CHILD_NW)]);
    MemoryManager::DELETE(_children[to_const_uint(ChildPosition::CHILD_NE)]);
    MemoryManager::DELETE(_children[to_const_uint(ChildPosition::CHILD_SW)]);
    MemoryManager::DELETE(_children[to_const_uint(ChildPosition::CHILD_SE)]);
}

void QuadtreeNode::Build(GFXDevice& context, U8 depth, const vec2<U32>& pos,
                         const vec2<U32>& HMsize, U32 minHMSize,
                         Terrain* const terrain, U32& chunkCount) {
    _minHMSize = minHMSize;
    U32 div = to_uint(std::pow(2.0f, to_float(depth)));
    vec2<U32> nodesize = HMsize / (div);
    if (nodesize.x % 2 == 0) nodesize.x++;
    if (nodesize.y % 2 == 0) nodesize.y++;
    vec2<U32> newsize = nodesize / 2;

    _terLoDOffset = (_minHMSize * 5.0f) / 100.0f;

    if (std::max(newsize.x, newsize.y) < _minHMSize) {
        _terrainChunk = MemoryManager_NEW TerrainChunk(context, terrain, this);
        _terrainChunk->load(depth, pos, _minHMSize, HMsize);
        chunkCount++;
        return;
    }

    // Create 4 children
    _children[to_const_uint(ChildPosition::CHILD_NW)] = MemoryManager_NEW QuadtreeNode();
    _children[to_const_uint(ChildPosition::CHILD_NE)] = MemoryManager_NEW QuadtreeNode();
    _children[to_const_uint(ChildPosition::CHILD_SW)] = MemoryManager_NEW QuadtreeNode();
    _children[to_const_uint(ChildPosition::CHILD_SE)] = MemoryManager_NEW QuadtreeNode();

    // Compute children bounding boxes
    const vec3<F32>& center = _boundingBox.getCenter();
    _children[to_const_uint(ChildPosition::CHILD_NW)]->setBoundingBox(
        BoundingBox(_boundingBox.getMin(), center));
    _children[to_const_uint(ChildPosition::CHILD_NE)]->setBoundingBox(
        BoundingBox(vec3<F32>(center.x, 0.0f, _boundingBox.getMin().z),
                    vec3<F32>(_boundingBox.getMax().x, 0.0f, center.z)));
    _children[to_const_uint(ChildPosition::CHILD_SW)]->setBoundingBox(
        BoundingBox(vec3<F32>(_boundingBox.getMin().x, 0.0f, center.z),
                    vec3<F32>(center.x, 0.0f, _boundingBox.getMax().z)));
    _children[to_const_uint(ChildPosition::CHILD_SE)]->setBoundingBox(
        BoundingBox(center, _boundingBox.getMax()));
    // Compute children positions
    vec2<U32> tNewHMpos[4];
    tNewHMpos[to_const_uint(ChildPosition::CHILD_NW)] = pos + vec2<U32>(0, 0);
    tNewHMpos[to_const_uint(ChildPosition::CHILD_NE)] =
        pos + vec2<U32>(newsize.x, 0);
    tNewHMpos[to_const_uint(ChildPosition::CHILD_SW)] =
        pos + vec2<U32>(0, newsize.y);
    tNewHMpos[to_const_uint(ChildPosition::CHILD_SE)] =
        pos + vec2<U32>(newsize.x, newsize.y);
    _children[to_const_uint(ChildPosition::CHILD_NW)]->Build(
        context, depth + 1, tNewHMpos[to_uint(ChildPosition::CHILD_NW)], HMsize,
        _minHMSize, terrain, chunkCount);
    _children[to_const_uint(ChildPosition::CHILD_NE)]->Build(
        context, depth + 1, tNewHMpos[to_uint(ChildPosition::CHILD_NE)], HMsize,
        _minHMSize, terrain, chunkCount);
    _children[to_const_uint(ChildPosition::CHILD_SW)]->Build(
        context, depth + 1, tNewHMpos[to_uint(ChildPosition::CHILD_SW)], HMsize,
        _minHMSize, terrain, chunkCount);
    _children[to_const_uint(ChildPosition::CHILD_SE)]->Build(
        context, depth + 1, tNewHMpos[to_uint(ChildPosition::CHILD_SE)], HMsize,
        _minHMSize, terrain, chunkCount);
}

bool QuadtreeNode::computeBoundingBox() {
    if (_terrainChunk != nullptr) {
        _boundingBox.setMin(vec3<F32>(_boundingBox.getMin().x,
                                      _terrainChunk->getMinHeight(),
                                      _boundingBox.getMin().z));
        _boundingBox.setMax(vec3<F32>(_boundingBox.getMax().x,
                                      _terrainChunk->getMaxHeight(),
                                      _boundingBox.getMax().z));
    }

    if (!isALeaf()) {
        for (I8 i = 0; i < 4; i++) {
            _children[i]->computeBoundingBox();

            if (_boundingBox.getMin().y >
                _children[i]->_boundingBox.getMin().y) {
                _boundingBox.setMin(
                    vec3<F32>(_boundingBox.getMin().x,
                              _children[i]->_boundingBox.getMin().y,
                              _boundingBox.getMin().z));
            }

            if (_boundingBox.getMax().y <
                _children[i]->_boundingBox.getMax().y) {
                _boundingBox.setMax(
                    vec3<F32>(_boundingBox.getMax().x,
                              _children[i]->_boundingBox.getMax().y,
                              _boundingBox.getMax().z));
            }
        }
    }

    _boundingSphere.fromBoundingBox(_boundingBox);
    return true;
}

U8 QuadtreeNode::getLoD(const SceneRenderState& state) const {
    F32 camDistance = _boundingSphere.getCenter().distance(Camera::activeCamera()->getEye()) - _terLoDOffset;
    F32 sphereRadius = _boundingSphere.getRadius();
    
    return camDistance >= sphereRadius
                        ? (camDistance >= (sphereRadius * 2) 
                                        ? 2 
                                        : 1)
                        : 0;
}

void QuadtreeNode::sceneUpdate(const U64 deltaTime, SceneGraphNode& sgn, SceneState& sceneState) {
    if (!isALeaf()) {
        _children[to_const_uint(ChildPosition::CHILD_NW)]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[to_const_uint(ChildPosition::CHILD_NE)]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[to_const_uint(ChildPosition::CHILD_SW)]->sceneUpdate(deltaTime, sgn, sceneState);
        _children[to_const_uint(ChildPosition::CHILD_SE)]->sceneUpdate(deltaTime, sgn, sceneState);
    }
}

bool QuadtreeNode::isInView(U32 options,
                            const SceneRenderState& sceneRenderState) const {
    if (BitCompare(options, to_const_uint(ChunkBit::CHUNK_BIT_TESTCHILDREN))) {
        const Camera& cam = *Camera::activeCamera();
        if (!BitCompare(options, to_const_uint(ChunkBit::CHUNK_BIT_SHADOWMAP))) {
            const vec3<F32>& eye = cam.getEye();
            F32 visibilityDistance = sceneRenderState.generalVisibility() + _boundingSphere.getRadius();
            if (_boundingSphere.getCenter().distance(eye) >
                visibilityDistance) {
                if (_boundingBox.nearestDistanceFromPointSquared(eye) -
                        _terLoDOffset >
                    std::min(visibilityDistance, cam.getZPlanes().y))
                    return false;
            }
        }
        if (!_boundingBox.containsPoint(cam.getEye())) {
            const Frustum& frust = cam.getFrustum();
            switch (frust.ContainsSphere(_boundingSphere.getCenter(),
                                         _boundingSphere.getRadius())) {
                case Frustum::FrustCollision::FRUSTUM_OUT:
                    return false;
                case Frustum::FrustCollision::FRUSTUM_IN:
                    options &= ~to_const_uint(ChunkBit::CHUNK_BIT_TESTCHILDREN);
                    break;
                case Frustum::FrustCollision::FRUSTUM_INTERSECT: {
                    switch (frust.ContainsBoundingBox(_boundingBox)) {
                        case Frustum::FrustCollision::FRUSTUM_IN:
                            options &= ~to_const_uint(ChunkBit::CHUNK_BIT_TESTCHILDREN);
                            break;
                        case Frustum::FrustCollision::FRUSTUM_OUT:
                            return false;
                    };  // inner switch
                };      // case Frustum::FrustCollision::FRUSTUM_INTERSECT
            };          // outer case
        }               // if
    }                   // ChunkBit::CHUNK_BIT_TESTCHILDREN option

    return true;
}

void QuadtreeNode::drawBBox(GFXDevice& context, GenericDrawCommands& commandsOut) {
    if (!_bbPrimitive) {
        _bbPrimitive = context.newIMP();
        _bbPrimitive->name("QuadtreeNodeBoundingBox");
        RenderStateBlock primitiveRenderState;
        _bbPrimitive->stateHash(primitiveRenderState.getHash());
    }

    _bbPrimitive->fromBox(_boundingBox.getMin(),
                          _boundingBox.getMax(),
                          vec4<U8>(0, 128, 255, 255));
    commandsOut.push_back(_bbPrimitive->toDrawCommand());

    if (!isALeaf()) {
        _children[to_const_uint(ChildPosition::CHILD_NW)]->drawBBox(context, commandsOut);
        _children[to_const_uint(ChildPosition::CHILD_NE)]->drawBBox(context, commandsOut);
        _children[to_const_uint(ChildPosition::CHILD_SW)]->drawBBox(context, commandsOut);
        _children[to_const_uint(ChildPosition::CHILD_SE)]->drawBBox(context, commandsOut);
    }
}

void QuadtreeNode::getBufferOffsetAndSize(U32 options,
                                          const SceneRenderState& sceneRenderState,
                                          vectorImpl<vec3<U32>>& chunkBufferData) const {
    if (isInView(options, sceneRenderState)) {
        if (isALeaf()) {
            assert(_terrainChunk);
            bool waterReflection = BitCompare(options, to_const_uint(ChunkBit::CHUNK_BIT_WATERREFLECTION));
            chunkBufferData.push_back(_terrainChunk->getBufferOffsetAndSize(waterReflection ? Config::TERRAIN_CHUNKS_LOD - 1 : getLoD(sceneRenderState)));
        } else {
            _children[to_const_uint(ChildPosition::CHILD_NW)]->getBufferOffsetAndSize(options, sceneRenderState, chunkBufferData);
            _children[to_const_uint(ChildPosition::CHILD_NE)]->getBufferOffsetAndSize(options, sceneRenderState, chunkBufferData);
            _children[to_const_uint(ChildPosition::CHILD_SW)]->getBufferOffsetAndSize(options, sceneRenderState, chunkBufferData);
            _children[to_const_uint(ChildPosition::CHILD_SE)]->getBufferOffsetAndSize(options, sceneRenderState, chunkBufferData);
        }
    }
}
};
#include "stdafx.h"

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
      _bbPrimitive(nullptr),
      _children(nullptr),
      _frustPlaneCache(-1)
{
    _targetChunkDimension = 0;
}

QuadtreeNode::~QuadtreeNode()
{
    MemoryManager::DELETE(_terrainChunk);
    MemoryManager::DELETE(_children);
}

void QuadtreeNode::Build(GFXDevice& context,
                         U8 depth,
                         const vec2<U32>& pos,
                         const vec2<U32>& HMsize,
                         U32 targetChunkDimension,
                         Terrain* const terrain,
                         U32& chunkCount)
{
    _targetChunkDimension = targetChunkDimension;
    U32 div = to_U32(std::pow(2.0f, to_F32(depth)));
    vec2<U32> nodesize = HMsize / (div);
    if (nodesize.x % 2 == 0) {
        nodesize.x++;
    }
    if (nodesize.y % 2 == 0) {
        nodesize.y++;
    }
    vec2<U32> newsize = nodesize / 2;

    if (std::max(newsize.x, newsize.y) <= _targetChunkDimension) {
        _terrainChunk = MemoryManager_NEW TerrainChunk(context, terrain, this);
        _terrainChunk->load(depth, pos, _targetChunkDimension, HMsize);
        chunkCount++;
    } else {
        // Create 4 children
        _children = MemoryManager_NEW QuadtreeChildren();

        QuadtreeChildren::QuadtreeNodes& childNodes = (*_children)();

        // Compute children bounding boxes
        const vec3<F32>& center = _boundingBox.getCenter();
        childNodes[to_base(ChildPosition::CHILD_NW)].setBoundingBox(BoundingBox(_boundingBox.getMin(), center));
        childNodes[to_base(ChildPosition::CHILD_NE)].setBoundingBox(BoundingBox(vec3<F32>(center.x, 0.0f, _boundingBox.getMin().z), vec3<F32>(_boundingBox.getMax().x, 0.0f, center.z)));
        childNodes[to_base(ChildPosition::CHILD_SW)].setBoundingBox(BoundingBox(vec3<F32>(_boundingBox.getMin().x, 0.0f, center.z), vec3<F32>(center.x, 0.0f, _boundingBox.getMax().z)));
        childNodes[to_base(ChildPosition::CHILD_SE)].setBoundingBox(BoundingBox(center, _boundingBox.getMax()));

        // Compute children positions
        vec2<U32> tNewHMpos[4];
        tNewHMpos[to_base(ChildPosition::CHILD_NW)] = pos + vec2<U32>(0, 0);
        tNewHMpos[to_base(ChildPosition::CHILD_NE)] = pos + vec2<U32>(newsize.x, 0);
        tNewHMpos[to_base(ChildPosition::CHILD_SW)] = pos + vec2<U32>(0, newsize.y);
        tNewHMpos[to_base(ChildPosition::CHILD_SE)] = pos + vec2<U32>(newsize.x, newsize.y);

        childNodes[to_base(ChildPosition::CHILD_NW)].Build(context, depth + 1, tNewHMpos[to_base(ChildPosition::CHILD_NW)], HMsize, _targetChunkDimension, terrain, chunkCount);
        childNodes[to_base(ChildPosition::CHILD_NE)].Build(context, depth + 1, tNewHMpos[to_base(ChildPosition::CHILD_NE)], HMsize, _targetChunkDimension, terrain, chunkCount);
        childNodes[to_base(ChildPosition::CHILD_SW)].Build(context, depth + 1, tNewHMpos[to_base(ChildPosition::CHILD_SW)], HMsize, _targetChunkDimension, terrain, chunkCount);
        childNodes[to_base(ChildPosition::CHILD_SE)].Build(context, depth + 1, tNewHMpos[to_base(ChildPosition::CHILD_SE)], HMsize, _targetChunkDimension, terrain, chunkCount);
    }
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
            QuadtreeNode& child = getChild(i);

            child.computeBoundingBox();

            if (_boundingBox.getMin().y >
                child._boundingBox.getMin().y) {
                _boundingBox.setMin(
                    vec3<F32>(_boundingBox.getMin().x,
                              child._boundingBox.getMin().y,
                              _boundingBox.getMin().z));
            }

            if (_boundingBox.getMax().y <
                child._boundingBox.getMax().y) {
                _boundingBox.setMax(
                    vec3<F32>(_boundingBox.getMax().x,
                              child._boundingBox.getMax().y,
                              _boundingBox.getMax().z));
            }
        }
    }

    _boundingSphere.fromBoundingBox(_boundingBox);
    return true;
}

U8 QuadtreeNode::getLoD(const vec3<F32>& eyePos) const {
    static const U32 TERRAIN_LOD0_SQ = Config::TERRAIN_LOD0 * Config::TERRAIN_LOD0;
    static const U32 TERRAIN_LOD1_SQ = Config::TERRAIN_LOD1 * Config::TERRAIN_LOD1;

    F32 cameraDistanceSQ = _boundingSphere.getCenter().distanceSquared(eyePos);

    return cameraDistanceSQ > TERRAIN_LOD0_SQ
                            ? cameraDistanceSQ > TERRAIN_LOD1_SQ ? 2 : 1
                            : 0;
}

void QuadtreeNode::sceneUpdate(const U64 deltaTimeUS, SceneGraphNode& sgn, SceneState& sceneState) {
    if (isALeaf()) {
        ACKNOWLEDGE_UNUSED(deltaTimeUS);
        ACKNOWLEDGE_UNUSED(sgn);
        ACKNOWLEDGE_UNUSED(sceneState);
    } else {
        getChild(ChildPosition::CHILD_NW).sceneUpdate(deltaTimeUS, sgn, sceneState);
        getChild(ChildPosition::CHILD_NE).sceneUpdate(deltaTimeUS, sgn, sceneState);
        getChild(ChildPosition::CHILD_SW).sceneUpdate(deltaTimeUS, sgn, sceneState);
        getChild(ChildPosition::CHILD_SE).sceneUpdate(deltaTimeUS, sgn, sceneState);
    }
}

bool QuadtreeNode::isInView(U32 options, const SceneRenderState& sceneRenderState) const {
    if (BitCompare(options, to_base(ChunkBit::CHUNK_BIT_TESTCHILDREN))) {
        const Camera& cam = *sceneRenderState.parentScene().playerCamera();
        F32 boundingRadius = _boundingSphere.getRadius();
        const vec3<F32>& boundingCenter = _boundingSphere.getCenter();

        if (!BitCompare(options, to_base(ChunkBit::CHUNK_BIT_SHADOWMAP))) {
            const vec3<F32>& eye = cam.getEye();
            F32 visibilityDistance = sceneRenderState.generalVisibility() + boundingRadius;
            if (boundingCenter.distance(eye) > visibilityDistance) {
                if (_boundingBox.nearestDistanceFromPointSquared(eye) > std::min(visibilityDistance, cam.getZPlanes().y)) {
                    return false;
                }
            }
        }

        if (!_boundingBox.containsPoint(cam.getEye())) {

            const Frustum& frust = cam.getFrustum();
            switch (frust.ContainsSphere(boundingCenter, boundingRadius, _frustPlaneCache)) {
                case Frustum::FrustCollision::FRUSTUM_OUT:
                    return false;
                case Frustum::FrustCollision::FRUSTUM_IN:
                    options &= ~to_base(ChunkBit::CHUNK_BIT_TESTCHILDREN);
                    break;
                case Frustum::FrustCollision::FRUSTUM_INTERSECT: {
                    switch (frust.ContainsBoundingBox(_boundingBox, _frustPlaneCache)) {
                        case Frustum::FrustCollision::FRUSTUM_IN:
                            options &= ~to_base(ChunkBit::CHUNK_BIT_TESTCHILDREN);
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

void QuadtreeNode::drawBBox(GFXDevice& context, RenderPackage& packageOut) {
    if (!_bbPrimitive) {
        _bbPrimitive = context.newIMP();
        _bbPrimitive->name("QuadtreeNodeBoundingBox");
        RenderStateBlock primitiveRenderState;
        PipelineDescriptor pipeDesc;
        pipeDesc._stateHash = primitiveRenderState.getHash();
        _bbPrimitive->pipeline(context.newPipeline(pipeDesc));
    }

    _bbPrimitive->fromBox(_boundingBox.getMin(),
                          _boundingBox.getMax(),
                          vec4<U8>(0, 128, 255, 255));

    packageOut.addCommandBuffer(_bbPrimitive->toCommandBuffer());

    if (!isALeaf()) {
        getChild(ChildPosition::CHILD_NW).drawBBox(context, packageOut);
        getChild(ChildPosition::CHILD_NE).drawBBox(context, packageOut);
        getChild(ChildPosition::CHILD_SW).drawBBox(context, packageOut);
        getChild(ChildPosition::CHILD_SE).drawBBox(context, packageOut);
    }
}
};
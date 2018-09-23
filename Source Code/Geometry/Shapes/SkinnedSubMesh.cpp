#include "stdafx.h"

#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "ECS/Components/Headers/AnimationComponent.h"

namespace Divide {

SkinnedSubMesh::SkinnedSubMesh(GFXDevice& context, ResourceCache& parentCache, size_t descriptorHash, const stringImpl& name)
    : SubMesh(context, parentCache, descriptorHash, name),
     _parentAnimatorPtr(nullptr)
{
    Object3D::setObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED);
}

SkinnedSubMesh::~SkinnedSubMesh()
{
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode& sgn) {
    if (_parentAnimatorPtr == nullptr) {
        _parentAnimatorPtr = _parentMesh->getAnimator();
        vec_size animationCount = _parentAnimatorPtr->animations().size();
        _boundingBoxesState.resize(animationCount, { BoundingBoxState::COUNT });
        _boundingBoxes.resize(animationCount);
    }

    sgn.get<AnimationComponent>()->updateAnimator(_parentAnimatorPtr);
    SubMesh::postLoad(sgn);
}

/// update possible animations
void SkinnedSubMesh::onAnimationChange(SceneGraphNode& sgn, I32 newIndex) {
    computeBBForAnimation(sgn, newIndex);

    Object3D::onAnimationChange(sgn, newIndex);
}

void SkinnedSubMesh::buildBoundingBoxesForAnim(const Task& parentTask,
                                               I32 animationIndex,
                                               AnimationComponent* const animComp) {
    if (animationIndex < 0) {
        return;
    }

    const vector<vectorBest<mat4<F32>>>& currentAnimation =
        animComp->getAnimationByIndex(animationIndex).transforms();

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();
    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = parentVB->getPartitionIndexCount(_geometryPartitionID);

    UniqueLock w_lock(_bbLock);
    BoundingBox& currentBB = _boundingBoxes.at(animationIndex);
    currentBB.reset();
    for (const vectorBest<mat4<F32> >& transforms : currentAnimation) {
        if (StopRequested(parentTask)) {
            return;
        }
        // loop through all vertex weights of all bones
        for (U32 j = 0; j < partitionCount; ++j) {
            if (StopRequested(parentTask)) {
                return;
            }

            U32 idx = parentVB->getIndex(j + partitionOffset);
            P32 ind = parentVB->getBoneIndices(idx);
            const vec4<F32>& wgh = parentVB->getBoneWeights(idx);
            const vec3<F32>& curentVert = parentVB->getPosition(idx);

            currentBB.add((wgh.x * (transforms[ind.b[0]] * curentVert)) +
                          (wgh.y * (transforms[ind.b[1]] * curentVert)) +
                          (wgh.z * (transforms[ind.b[2]] * curentVert)) +
                          (wgh.w * (transforms[ind.b[3]] * curentVert)));
        }
    }
}

void SkinnedSubMesh::updateBB(I32 animIndex) {
    UniqueLock r_lock(_bbLock);
    _boundingBox.set(_boundingBoxes[animIndex]);
    setBoundsChanged();
}

void SkinnedSubMesh::computeBBForAnimation(SceneGraphNode& sgn, I32 animIndex) {
    // Attempt to get the map of BBs for the current animation
    UniqueLock w_lock(_bbStateLock);
    BoundingBoxState& state = _boundingBoxesState[animIndex];

    if (state != BoundingBoxState::COUNT) {
        if (state == BoundingBoxState::Computed) {
            updateBB(animIndex);
        }
        return;
    }

    _boundingBoxesState[animIndex] = BoundingBoxState::Computing;
    AnimationComponent* animComp = sgn.get<AnimationComponent>();
    auto bbBuildStart = [this, animIndex, animComp](const Task& parentTask) {
        buildBoundingBoxesForAnim(parentTask, animIndex, animComp);
    };

    auto bbBuildComplete = [this, animComp, animIndex]() {
        UniqueLock w_lock(_bbStateLock);
        _boundingBoxesState[animIndex] = BoundingBoxState::Computed;
        // We could've changed the animation while waiting for this task to end
        if (animComp->animationIndex() == animIndex) {
            updateBB(animIndex);
        }
    };

    CreateTask(_context.parent().platformContext(), bbBuildStart).startTask(bbBuildComplete);
}

};
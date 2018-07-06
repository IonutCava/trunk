#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/SceneAnimator.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

namespace Divide {

SkinnedSubMesh::SkinnedSubMesh(const stringImpl& name)
    : SubMesh(name, Object3D::ObjectFlag::OBJECT_FLAG_SKINNED),
    _parentAnimatorPtr(nullptr)
{
}

SkinnedSubMesh::~SkinnedSubMesh()
{
    for (Task_weak_ptr task_ptr : _bbBuildTasks) {
        Task_ptr task = task_ptr.lock();
        if (task) {
            task->stopTask();
            while (!task->isFinished());
        }
    }
    _bbBuildTasks.clear();
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode& sgn) {
    if (_parentAnimatorPtr == nullptr) {
        _parentAnimatorPtr = _parentMesh->getAnimator();
    }

    sgn.setComponent(
        SGNComponent::ComponentType::ANIMATION,
        new AnimationComponent(*_parentAnimatorPtr, sgn));

    SubMesh::postLoad(sgn);
}

/// update possible animations
bool SkinnedSubMesh::updateAnimations(SceneGraphNode& sgn) {
    assert(sgn.getComponent<AnimationComponent>());

    return getBoundingBoxForCurrentFrame(sgn);
}

void SkinnedSubMesh::buildBoundingBoxesForAnimCompleted(U32 animationIndex) {
    boundingBoxPerAnimationStatus::iterator it1 = _boundingBoxesAvailable.find(animationIndex);
    boundingBoxPerAnimationStatus::iterator it2 = _boundingBoxesComputing.find(animationIndex);
    it1->second = true;
    it2->second = false;
}

void SkinnedSubMesh::buildBoundingBoxesForAnim(
    U32 animationIndex, AnimationComponent* const animComp) {

    boundingBoxPerAnimationStatus::iterator it = _boundingBoxesComputing.find(animationIndex);
    if (it != std::end(_boundingBoxesComputing) && it->second == true) {
        return;
    }
    
    std::atomic_bool& currentBBStatus = _boundingBoxesAvailable[animationIndex];
    std::atomic_bool& currentBBComputing = _boundingBoxesComputing[animationIndex];
    currentBBComputing = true;
    currentBBStatus = false;

    const AnimEvaluator& currentAnimation =
        animComp->GetAnimationByIndex(animationIndex);

    boundingBoxPerFrame& currentBBs = _boundingBoxes[animationIndex];
    // We might need to recompute BBs so clear any possible old values
    currentBBs.clear();

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();
    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionID);
    U32 partitionCount = parentVB->getPartitionCount(_geometryPartitionID) +
                         partitionOffset;
                         
    U32 frameCount = to_uint(animComp->frameCount(animationIndex));

    for (U32 i = 0; i < frameCount; ++i) {
        BoundingBox& bb = currentBBs[i];

        const vectorImpl<mat4<F32> >& transforms = currentAnimation.transforms(i);
        // loop through all vertex weights of all bones
        for (U32 j = partitionOffset; j < partitionCount; ++j) {
            U32 idx = parentVB->getIndex(j);
            P32 ind = parentVB->getBoneIndices(idx);
            const vec4<F32>& wgh = parentVB->getBoneWeights(idx);
            const vec3<F32>& curentVert = parentVB->getPosition(idx);

            F32 fwgh = 1.0f - (wgh.x + wgh.y + wgh.z);
            bb.Add((wgh.x * (transforms[ind.b[0]] * curentVert)) +
                   (wgh.y * (transforms[ind.b[1]] * curentVert)) +
                   (wgh.z * (transforms[ind.b[2]] * curentVert)) +
                   (fwgh  * (transforms[ind.b[3]] * curentVert)));
        }

        bb.setComputed(true);
    }
}

bool SkinnedSubMesh::getBoundingBoxForCurrentFrame(SceneGraphNode& sgn) {
    AnimationComponent* animComp = sgn.getComponent<AnimationComponent>();
    // If anymations are paused or unavailable, keep the current BB
    if (!animComp->playAnimations()) {
        return true;
    }
    // Attempt to get the map of BBs for the current animation
    U32 animationIndex = animComp->animationIndex();

    boundingBoxPerAnimationStatus::iterator it1 = _boundingBoxesAvailable.find(animationIndex);
    bool bbAvailable = !(it1 == std::end(_boundingBoxesAvailable) || it1->second == false);
    
    if (!bbAvailable) {
        boundingBoxPerAnimationStatus::iterator it2 = _boundingBoxesComputing.find(animationIndex);
        if (it2 == std::end(_boundingBoxesComputing) || it2->second == false) {
            DELEGATE_CBK<> buildBB = DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnim,
                                                   this, animationIndex, animComp);
            DELEGATE_CBK<> builBBComplete = DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnimCompleted,
                                                          this, animationIndex);
            _bbBuildTasks.push_back(Application::getInstance()
                .getKernel()
                .AddTask(1, 1, buildBB, builBBComplete));
            _bbBuildTasks.back().lock()->startTask(Task::TaskPriority::DONT_CARE);
        }

        return true;
    }

    boundingBoxPerAnimation::const_iterator it3 = _boundingBoxes.find(animationIndex);
    // If the BBs are computed, set the BB for the current frame as the node BB
    if (it3 != std::end(_boundingBoxes)) {
        sgn.setInitialBoundingBox(it3->second.find(animComp->frameIndex())->second);
    }

    return true;
}
};
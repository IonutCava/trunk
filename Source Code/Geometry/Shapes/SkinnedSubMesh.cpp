#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/AnimationController.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

namespace Divide {

const static bool USE_MUTITHREADED_LOADING = false;

SkinnedSubMesh::SkinnedSubMesh(const stringImpl& name) : SubMesh(name, Object3D::OBJECT_FLAG_SKINNED)
{
    _animator = MemoryManager_NEW SceneAnimator();
    _buildingBoundingBoxes = false;
}

SkinnedSubMesh::~SkinnedSubMesh()
{
    MemoryManager::DELETE( _animator );
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode* const sgn){
    sgn->setComponent(SGNComponent::SGN_COMP_ANIMATION, MemoryManager_NEW AnimationComponent(_animator, sgn));
    SubMesh::postLoad(sgn);
}

/// update possible animations
bool SkinnedSubMesh::updateAnimations(SceneGraphNode* const sgn){
    assert(sgn->getComponent<AnimationComponent>());

    return getBoundingBoxForCurrentFrame(sgn);
}

void SkinnedSubMesh::buildBoundingBoxesForAnimCompleted(U32 animationIndex) {
    _buildingBoundingBoxes = false;
}

void SkinnedSubMesh::buildBoundingBoxesForAnim(U32 animationIndex, AnimationComponent* const animComp) {
    typedef hashMapImpl<U32 /*frame index*/, BoundingBox>  animBBs;
    const AnimEvaluator& currentAnimation = animComp->GetAnimationByIndex(animationIndex);

    animBBs currentBBs;

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();
    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionId);
    U32 partitionCount  = parentVB->getPartitionCount(_geometryPartitionId) + partitionOffset;

    const vectorImpl<vec3<F32> >& verts   = parentVB->getPosition();
    const vectorImpl<vec4<U8>  >& indices = parentVB->getBoneIndices();
    const vectorImpl<vec4<F32> >& weights = parentVB->getBoneWeights();

    I32 frameCount = animComp->frameCount(animationIndex);
    //#pragma omp parallel for
    for (I32 i = 0; i < frameCount; ++i) {
        BoundingBox& bb = currentBBs[i];
        bb.reset();

        const vectorImpl<mat4<F32> >& transforms = currentAnimation.GetTransformsConst(static_cast<U32>(i));

        // loop through all vertex weights of all bones
        for (U32 j = partitionOffset; j < partitionCount; ++j) {
            U32 idx = parentVB->getIndex(j);
            const vec4<U8>&  ind = indices[idx];
            const vec4<F32>& wgh = weights[idx];
            const vec3<F32>& curentVert = verts[idx];

            F32 fwgh = 1.0f - ( wgh.x + wgh.y + wgh.z );

            bb.Add((wgh.x * (transforms[ind.x] * curentVert)) +
                   (wgh.y * (transforms[ind.y] * curentVert)) +
                   (wgh.z * (transforms[ind.z] * curentVert)) +
                   (fwgh  * (transforms[ind.w] * curentVert)));
        }

        bb.setComputed(true);
    }

    animBBs& oldAnim = animComp->getBBoxesForAnimation(animationIndex);

    oldAnim.clear();
    oldAnim.swap(currentBBs);
}

bool SkinnedSubMesh::getBoundingBoxForCurrentFrame(SceneGraphNode* const sgn){
    AnimationComponent* animComp = sgn->getComponent<AnimationComponent>();
    if (!animComp->playAnimations()) {
        return _buildingBoundingBoxes;
    }

    if (_buildingBoundingBoxes) {
        return true;
    }

    U32 animationIndex = animComp->animationIndex();
    AnimationComponent::boundingBoxPerFrame& animBB = animComp->getBBoxesForAnimation(animationIndex);
    
    if (animBB.empty()){
        if (!_buildingBoundingBoxes) {
            _buildingBoundingBoxes = true;
            if (USE_MUTITHREADED_LOADING) {
                Kernel* kernel = Application::getInstance().getKernel();
                Task* task = kernel->AddTask(1,
                                             1,
                                             DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnim,
                                                           this,
                                                           animationIndex,
                                                           animComp),
                                             DELEGATE_BIND(&SkinnedSubMesh::buildBoundingBoxesForAnimCompleted,
                                                           this,
                                                           animationIndex));
                task->startTask();
            } else {
                buildBoundingBoxesForAnim(animationIndex, animComp);
                buildBoundingBoxesForAnimCompleted(animationIndex);
            }
        }

    } else {
        sgn->setInitialBoundingBox(animBB[animComp->frameIndex()]);
    }

    return true;
}

};
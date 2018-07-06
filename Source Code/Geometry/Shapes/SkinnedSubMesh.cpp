#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/AnimationController.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

SkinnedSubMesh::SkinnedSubMesh(const std::string& name) : SubMesh(name, Object3D::OBJECT_FLAG_SKINNED)
{
   _animator =  New SceneAnimator();
}

SkinnedSubMesh::~SkinnedSubMesh()
{
    SAFE_DELETE(_animator);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode* const sgn){
    sgn->setComponent(SGNComponent::SGN_COMP_ANIMATION, New AnimationComponent(_animator, sgn));

    SubMesh::postLoad(sgn);
}

/// update possible animations
bool SkinnedSubMesh::updateAnimations(SceneGraphNode* const sgn){
    assert(sgn->getComponent<AnimationComponent>());

    return getBoundingBoxForCurrentFrame(sgn);
}

bool SkinnedSubMesh::getBoundingBoxForCurrentFrame(SceneGraphNode* const sgn){
    AnimationComponent* animComp = sgn->getComponent<AnimationComponent>();
    if (!animComp->playAnimations()) return false;

    VertexBuffer* parentVB = _parentMesh->getGeometryVB();

    U32 partitionOffset = parentVB->getPartitionOffset(_geometryPartitionId);
    U32 partitionCount  = parentVB->getPartitionCount(_geometryPartitionId) + partitionOffset;

    AnimationComponent::boundingBoxPerFrame& animBB = animComp->getBBoxesForAnimation(animComp->animationIndex());
    
    if (animBB.empty()){
        const vectorImpl<vec3<F32> >& verts   = parentVB->getPosition();
        const vectorImpl<vec4<U8>  >& indices = parentVB->getBoneIndices();
        const vectorImpl<vec4<F32> >& weights = parentVB->getBoneWeights();

        //#pragma omp parallel for
        for (I32 i = 0; i < animComp->frameCount(); i++){
            BoundingBox& bb = animBB[i];
            bb.reset();

            const vectorImpl<mat4<F32> >& transforms = animComp->transformsByIndex(i);

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
    }

    sgn->setInitialBoundingBox(animBB[animComp->frameIndex()]);
    return true;
}
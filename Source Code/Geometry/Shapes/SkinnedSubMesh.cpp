#include "Headers/SkinnedSubMesh.h"
#include "Headers/Mesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"

#include "Geometry/Animations/Headers/AnimationController.h"
#include "Graphs/Components/Headers/AnimationComponent.h"

SkinnedSubMesh::SkinnedSubMesh(const std::string& name) : SubMesh(name, Object3D::OBJECT_FLAG_SKINNED),
                                                          _softwareSkinning(false)
{
   _animator =  New SceneAnimator();
}

SkinnedSubMesh::~SkinnedSubMesh()
{
    SAFE_DELETE(_animator);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode* const sgn){
    //sgn->getTransform()->setTransforms(_localMatrix);
    // If the mesh has animation data, use dynamic VBO's if we use software skinning
    _softwareSkinning = ParamHandler::getInstance().getParam<bool>("mesh.useSoftwareSkinning", false);
    getGeometryVBO()->Create(!_softwareSkinning);
    sgn->setComponent(SGNComponent::SGN_COMP_ANIMATION, New AnimationComponent(_animator, sgn));

    Object3D::postLoad(sgn);
}

/// Called from SceneGraph "sceneUpdate"
void SkinnedSubMesh::sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState){
    updateAnimations(sgn);
    SceneNode::sceneUpdate(deltaTime, sgn, sceneState);
}

// update possible animations
void SkinnedSubMesh::updateAnimations(SceneGraphNode* const sgn){
    AnimationComponent* animComp = sgn->getComponent<AnimationComponent>();
    assert(animComp);

    updateBBatCurrentFrame(sgn);

    //Software skinning
    if (!_softwareSkinning || !animComp->playAnimations()) return;

    if(_origVerts.empty()){
        for(U32 i = 0; i < _geometry->getPosition().size(); i++){
            _origVerts.push_back(_geometry->getPosition()[i]);
            _origNorms.push_back(_geometry->getNormal()[i]);
        }
    }

    const vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
    const vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();
    const vectorImpl<mat4<F32> >& trans   = animComp->animationTransforms();

    vec4<F32> pos1, pos2, pos3, pos4, finalPos;
    vec4<F32> norm1, norm2, norm3, norm4, finalNorm;
    // loop through all vertex weights of all bones
    for( size_t i = 0; i < _geometry->getPosition().size(); ++i) {
        const vec3<F32>& pos  = _origVerts[i];
        const vec3<F32>& norm = _origNorms[i];
        const vec4<U8>&  ind  = indices[i];
        const vec4<F32>& wgh  = weights[i];

        F32 fwgh = 1.0f - ( wgh.x + wgh.y + wgh.z );

        pos1.set(wgh.x * (trans[ind.x] * pos));
        pos2.set(wgh.y * (trans[ind.y] * pos));
        pos3.set(wgh.z * (trans[ind.z] * pos));
        pos4.set(fwgh  * (trans[ind.w] * pos));
        finalPos = pos1 + pos2 + pos3 + pos4;
        _geometry->modifyPositionValue((U32)i, finalPos.xyz());

        norm1.set(wgh.x * (trans[ind.x] * norm));
        norm2.set(wgh.y * (trans[ind.y] * norm));
        norm3.set(wgh.z * (trans[ind.z] * norm));
        norm4.set(fwgh  * (trans[ind.w] * norm));
        finalNorm = norm1 + norm2 + norm3 + norm4;
        _geometry->modifyPositionValue((U32)i, finalNorm.xyz());
    }

    _geometry->queueRefresh();
}

void SkinnedSubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
    AnimationComponent* animComp = sgn->getComponent<AnimationComponent>();
    if (!animComp || !animComp->playAnimations()) return;

    U32 currentAnimationID = animComp->animationIndex();
    U32 currentFrameIndex  = animComp->frameIndex();

    if(_boundingBoxes.find(currentAnimationID) == _boundingBoxes.end()){
        _bbsPerFrame.clear();

        const vectorImpl<vec3<F32> >& verts   = _geometry->getPosition();
        const vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
        const vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();

        vec4<F32> pos1,pos2,pos3,pos4;
        BoundingBox bb;

        for (U16 i = 0; i < animComp->frameCount(); i++){
            bb.reset();

            const vectorImpl<mat4<F32> >& transforms = animComp->transformsByIndex(i);

            /// loop through all vertex weights of all bones
            for( size_t j = 0; j < verts.size(); ++j) {
                const vec4<U8>&  ind        = indices[j];
                const vec4<F32>& wgh        = weights[j];
                const vec3<F32>& curentVert = verts[j];

                F32 fwgh = 1.0f - ( wgh.x + wgh.y + wgh.z );

                pos1.set(wgh.x * (transforms[ind.x] * curentVert));
                pos2.set(wgh.y * (transforms[ind.y] * curentVert));
                pos3.set(wgh.z * (transforms[ind.z] * curentVert));
                pos4.set(fwgh  * (transforms[ind.w] * curentVert));
                bb.Add( pos1 + pos2 + pos3 + pos4 );
            }

            bb.setComputed(true);
            _bbsPerFrame[i] = bb;
        }
        _boundingBoxes.insert(std::make_pair(currentAnimationID,_bbsPerFrame));
    }

    const BoundingBox& bb1 = _boundingBoxes[currentAnimationID][currentFrameIndex];
    const BoundingBox& bb2 = sgn->getBoundingBoxConst();
    if(!bb1.Compare(bb2)){
        sgn->setInitialBoundingBox(bb1);
    }
}
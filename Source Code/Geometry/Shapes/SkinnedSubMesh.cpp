#include "Headers/SkinnedSubMesh.h"
#include "Headers/SkinnedMesh.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Animations/Headers/AnimationController.h"

SkinnedSubMesh::~SkinnedSubMesh()
{
    SAFE_DELETE(_animator);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SkinnedSubMesh::postLoad(SceneGraphNode* const sgn){
    //sgn->getTransform()->setTransforms(_localMatrix);
    /// If the mesh has animation data, use dynamic VBO's if we use software skinning
    getGeometryVBO()->Create(!_softwareSkinning);
    Object3D::postLoad(sgn);
}

void SkinnedSubMesh::sceneUpdate(const U32 sceneTime, SceneGraphNode* const sgn, SceneState& sceneState){
    if(_playAnimations)
        sgn->animationTransforms(_animator->GetTransforms(_deltaTime));
    else
        sgn->animationTransforms().clear();

    Object3D::sceneUpdate(sceneTime,sgn,sceneState);
}

/// Create a mesh animator from assimp imported data
bool SkinnedSubMesh::createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer){
    assert(scene != NULL);
    /// Delete old animator if any
    SAFE_UPDATE(_animator,New SceneAnimator());
    _animator->Init(scene,subMeshPointer);

    return true;
}

void SkinnedSubMesh::renderSkeleton(SceneGraphNode* const sgn){
    if(!_skeletonAvailable || !GET_ACTIVE_SCENE()->renderState().drawSkeletons()) return;
    // update possible animation
    assert(_animator != NULL);

    _animator->setGlobalMatrix(sgn->getTransform()->getGlobalMatrix());
    _animator->RenderSkeleton(_deltaTime);
}

// update possible animations
void SkinnedSubMesh::updateAnimations(D32 timeIndex, SceneGraphNode* const sgn){
    _skeletonAvailable = false;
    _deltaTime = timeIndex;

    if(!_animator ||!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE))
        return;

    if(ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")){
        _playAnimations = dynamic_cast<SkinnedMesh*>(getParentMesh())->playAnimations();
    }else{
        _playAnimations = false;
        return;
    }

    //All animation data is valid, so we have a skeleton to render if needed
    _skeletonAvailable = true;

    ///Software skinning
    if(!_softwareSkinning)
        return;

    if(_origVerts.empty()){
        for(U32 i = 0; i < _geometry->getPosition().size(); i++){
            _origVerts.push_back(_geometry->getPosition()[i]);
            _origNorms.push_back(_geometry->getNormal()[i]);
        }
    }

    const vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
    const vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();
    const vectorImpl<mat4<F32> >& trans   = sgn->animationTransforms();

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
        _geometry->modifyPositionValue(i, finalPos.xyz());

        norm1.set(wgh.x * (trans[ind.x] * norm));
        norm2.set(wgh.y * (trans[ind.y] * norm));
        norm3.set(wgh.z * (trans[ind.z] * norm));
        norm4.set(fwgh  * (trans[ind.w] * norm));
        finalNorm = norm1 + norm2 + norm3 + norm4;
        _geometry->modifyPositionValue(i, finalNorm.xyz());
    }

    _geometry->queueRefresh();
}

void SkinnedSubMesh::updateTransform(SceneGraphNode* const sgn){
    Transform* transform = sgn->getTransform();
    //a submesh should always have a transform
    assert(transform);
    if(transform->isDirty() && _animator != NULL  && _playAnimations){
        _animator->setGlobalMatrix(sgn->getTransform()->getGlobalMatrix());
    }
    transform = sgn->getParent()->getTransform();
    //a mesh should always have a transform
    assert(transform);

    sgn->updateBoundingBoxTransform(transform->getGlobalMatrix());
}

void SkinnedSubMesh::preFrameDrawEnd(SceneGraphNode* const sgn){
    renderSkeleton(sgn);
    SceneNode::preFrameDrawEnd(sgn);
}

void SkinnedSubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
    if(!_animator || !_playAnimations) return;

    _currentAnimationID = _animator->GetAnimationIndex();
    _currentFrameIndex  = _animator->GetFrameIndex();

    if(_boundingBoxes.find(_currentAnimationID) == _boundingBoxes.end()){
        _bbsPerFrame.clear();

        const vectorImpl<vec3<F32> >& verts   = _geometry->getPosition();
        const vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
        const vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();

        vec4<F32> pos1,pos2,pos3,pos4;
        BoundingBox bb;

        for(U16 i = 0; i < _animator->GetFrameCount(); i++){
            bb.reset();

            const vectorImpl<mat4<F32> >& transforms = _animator->GetTransformsByIndex(i);

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
        _boundingBoxes.insert(std::make_pair(_currentAnimationID,_bbsPerFrame));
    }

    const BoundingBox& bb1 = _boundingBoxes[_currentAnimationID][_currentFrameIndex];
    const BoundingBox& bb2 = sgn->getBoundingBox();
    if(!bb1.Compare(bb2)){
        sgn->updateBoundingBox(bb1);
        SceneNode::computeBoundingBox(sgn);
        sgn->getParent()->updateBB(true);
    }
}
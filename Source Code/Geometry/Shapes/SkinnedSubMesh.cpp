#include "Headers/SkinnedSubMesh.h"
#include "Headers/SkinnedMesh.h"
#include "Core/Headers/ParamHandler.h"
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


/// Create a mesh animator from assimp imported data
bool SkinnedSubMesh::createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer){
	assert(scene != NULL);
	/// Delete old animator if any
	SAFE_DELETE(_animator);
	_animator = New SceneAnimator();
	_animator->Init(scene,subMeshPointer);

	return true;
}

void SkinnedSubMesh::renderSkeleton(SceneGraphNode* const sgn){
	// update possible animation
	if(_skeletonAvailable) {
		assert(_animator != NULL);
		_animator->setGlobalMatrix(sgn->getTransform()->getGlobalMatrix());
		_animator->RenderSkeleton(_deltaTime);
	}
}

// update possible animation
void SkinnedSubMesh::updateAnimations(D32 timeIndex){

	_skeletonAvailable = false;
	if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
		if (_animator != NULL && dynamic_cast<SkinnedMesh*>(getParentMesh())->playAnimations()) {
			 _deltaTime = timeIndex;
			  //set the bone animation to the specified timestamp
			 _transforms = _animator->GetTransforms(_deltaTime);
			 //All animation data is valid, so we have a skeleton to render if needed
			 _skeletonAvailable = true;
		}
	}
}

void SkinnedSubMesh::updateTransform(SceneGraphNode* const sgn){
	if(_animator != NULL  && dynamic_cast<SkinnedMesh*>(getParentMesh())->playAnimations() && !_transforms.empty()){
		_animator->setGlobalMatrix(sgn->getTransform()->getGlobalMatrix());
	}
}

void SkinnedSubMesh::preFrameDrawEnd(SceneGraphNode* const sgn){
	if(GET_ACTIVE_SCENE()->renderState()->drawSkeletons()) {
		renderSkeleton(sgn);
	}
	SceneNode::preFrameDrawEnd(sgn);
}

void SkinnedSubMesh::setSpecialShaderConstants(ShaderProgram* const shader){
	if(!_transforms.empty()){
		shader->Uniform("hasAnimations", true);	
		shader->Uniform("boneTransforms", _transforms);
	}
}

void SkinnedSubMesh::onDraw(){
		///Software skinning
	if(_softwareSkinning){

		if(_animator != NULL  && dynamic_cast<SkinnedMesh*>(getParentMesh())->playAnimations()){

			if(GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)){
				if(!_transforms.empty()){
					if(_origVerts.empty()){
						for(U32 i = 0; i < _geometry->getPosition().size(); i++){
							_origVerts.push_back(_geometry->getPosition()[i]);
						}
					}
					vectorImpl<vec3<F32> >  verts   = _geometry->getPosition();
					//vectorImpl<vec3<F32> >& normals = _geometry->getNormal();
					vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
					vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();

					/// loop through all vertex weights of all bones 
					for( size_t i = 0; i < verts.size(); ++i) { 
						vec3<F32>& pos = _origVerts[i];
						//vec3<F32>& nor = normals[i];

						vec4<U8>&  ind = indices[i];
						vec4<F32>& wgh = weights[i];
						F32 finalWeight = 1.0f - ( wgh.x + wgh.y + wgh.z );

						vec4<F32> pos1 = wgh.x       * (_transforms[ind.x] * pos);
						vec4<F32> pos2 = wgh.y       * (_transforms[ind.y] * pos);
						vec4<F32> pos3 = wgh.z       * (_transforms[ind.z] * pos);
						vec4<F32> pos4 = finalWeight * (_transforms[ind.w] * pos);

						vec4<F32> finalPosition =  pos1 + pos2 + pos3 + pos4;
						verts[i] = finalPosition;

					}
					_geometry->queueRefresh();
				}
			}	
		}
	}
	Object3D::onDraw();
}

void SkinnedSubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
	if(!_animator) return;
	if(!dynamic_cast<SkinnedMesh*>(getParentMesh())->playAnimations()) return;
	if(!ParamHandler::getInstance().getParam<bool>("mesh.playAnimations")) return;

	_currentAnimationID = _animator->GetAnimationIndex();
	_currentFrameIndex = _animator->GetFrameIndex();

	if(_boundingBoxes.find(_currentAnimationID) == _boundingBoxes.end()){
		tempHolder.clear();
		vectorImpl<vec3<F32> > verts   = _geometry->getPosition();
		vectorImpl<vec4<U8>  >& indices = _geometry->getBoneIndices();
		vectorImpl<vec4<F32> >& weights = _geometry->getBoneWeights();

		for(U16 i = 0; i < _animator->GetFrameCount(); i++){

			vectorImpl<mat4<F32> > transforms = _animator->GetTransformsByIndex(i);

			BoundingBox bb(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));

			/// loop through all vertex weights of all bones 
			for( size_t j = 0; j < _geometry->getPosition().size(); ++j) { 
				vec4<U8>&  ind = indices[j];
				vec4<F32>& wgh = weights[j];
				F32 finalWeight = 1.0f - ( wgh.x + wgh.y + wgh.z );

				vec4<F32> pos1 = wgh.x       * (transforms[ind.x] * _geometry->getPosition(j));
				vec4<F32> pos2 = wgh.y       * (transforms[ind.y] * _geometry->getPosition(j));
				vec4<F32> pos3 = wgh.z       * (transforms[ind.z] * _geometry->getPosition(j));
				vec4<F32> pos4 = finalWeight * (transforms[ind.w] * _geometry->getPosition(j));
				vec4<F32> finalPosition =  pos1 + pos2 + pos3 + pos4;
				bb.Add( finalPosition );
			}
			bb.isComputed() = true;
			tempHolder[i] = bb;
		}
		_boundingBoxes.insert(std::make_pair(_currentAnimationID,tempHolder));
	}

	BoundingBox& bb1 = _boundingBoxes[_currentAnimationID][_currentFrameIndex];
	BoundingBox& bb2 = sgn->getBoundingBox();
	if(!bb1.Compare(bb2)){
		bb2 = bb1;
		SceneNode::computeBoundingBox(sgn);
		sgn->getParent()->updateBB(true);
	}
}
#include "Headers/SubMesh.h"
#include "Headers/Mesh.h"
#include "Managers/Headers/ResourceManager.h"
#include "Geometry/Animations/Headers/AnimationController.h"

SubMesh::~SubMesh(){
	SAFE_DELETE(_animator);
}

bool SubMesh::computeBoundingBox(SceneGraphNode* const sgn){
	BoundingBox& bb = sgn->getBoundingBox();
	if(bb.isComputed()) return true;
	bb.set(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));

	std::vector<vec3<F32> >&	tPosition	= _geometry->getPosition();

	for(U32 i=0; i < tPosition.size(); i++){
		bb.Add( tPosition[i] );
	}

	return SceneNode::computeBoundingBox(sgn);
}

/// After we loaded our mesh, we need to add submeshes as children nodes
void SubMesh::postLoad(SceneGraphNode* const sgn){
	//sgn->getTransform()->setTransforms(_localMatrix);
	/// If the mesh has animation data, use dynamic VBO's if we use software skinning
	getGeometryVBO()->Create(!_softwareSkinning);
	Object3D::postLoad(sgn);
}

bool SubMesh::unload(){
	return SceneNode::unload();
}

void SubMesh::setSceneMatrix(const mat4<F32>& sceneMatrix){
	_sceneRootMatrix = sceneMatrix;
}

void SubMesh::updateBBatCurrentFrame(SceneGraphNode* const sgn){
	if(_animator != NULL  && getParentMesh()->_playAnimations){
		_currentAnimationID = _animator->GetAnimationIndex();
		_currentFrameIndex = _animator->GetFrameIndex();
		if(_boundingBoxes.find(_currentAnimationID) == _boundingBoxes.end()){
			tempHolder.clear();
			std::vector<vec3<F32> >& verts   = _geometry->getPosition();
			std::vector<vec4<U8>  >& indices = _geometry->getBoneIndices();
			std::vector<vec4<F32> >& weights = _geometry->getBoneWeights();

			for(U16 i = 0; i < _animator->GetFrameCount(); i++){

				std::vector<mat4<F32> > transforms = _animator->GetTransformsByIndex(i);

				BoundingBox bb(vec3<F32>(100000.0f, 100000.0f, 100000.0f),vec3<F32>(-100000.0f, -100000.0f, -100000.0f));

				/// loop through all vertex weights of all bones 
				for( size_t j = 0; j < verts.size(); ++j) { 
					vec4<U8>&  ind = indices[j];
					vec4<F32>& wgh = weights[j];
					F32 finalWeight = 1.0f - ( wgh.x + wgh.y + wgh.z );

					vec4<F32> pos1 = wgh.x       * (transforms[ind.x] * verts[j]);
					vec4<F32> pos2 = wgh.y       * (transforms[ind.y] * verts[j]);
					vec4<F32> pos3 = wgh.z       * (transforms[ind.z] * verts[j]);
					vec4<F32> pos4 = finalWeight * (transforms[ind.w] * verts[j]);
					vec4<F32> finalPosition =  pos1 + pos2 + pos3 + pos4;
					bb.Add( finalPosition );
				}
				bb.isComputed() = true;
				tempHolder[i] = bb;
			}
			_boundingBoxes.insert(std::make_pair(_currentAnimationID,tempHolder));
		}else{
			BoundingBox& bb1 = _boundingBoxes[_currentAnimationID][_currentFrameIndex];
			BoundingBox& bb2 = sgn->getBoundingBox();
			if(!bb1.Compare(bb2)){
				bb2 = bb1;
				SceneNode::computeBoundingBox(sgn);
			}
		}
	}
}

void SubMesh::updateTransform(SceneGraphNode* const sgn){
	if(_animator != NULL  && getParentMesh()->_playAnimations && !_transforms.empty()){
		_animator->setGlobalMatrix(sgn->getTransform()->getGlobalMatrix());
	}
}

void SubMesh::onDraw(){
	///Software skinning
	if(_softwareSkinning){

		if(_animator != NULL  && getParentMesh()->_playAnimations){

			if(GFX_DEVICE.getRenderStage() == FINAL_STAGE ){

				if(!_transforms.empty()){
					if(_origVerts.empty()){
						for(U32 i = 0; i < _geometry->getPosition().size(); i++){
							_origVerts.push_back(_geometry->getPosition()[i]);
						}
					}
					std::vector<vec3<F32> >& verts   = _geometry->getPosition();
					//std::vector<vec3<F32> >& normals = _geometry->getNormal();
					std::vector<vec4<U8>  >& indices = _geometry->getBoneIndices();
					std::vector<vec4<F32> >& weights = _geometry->getBoneWeights();

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
					_geometry->Refresh();
				}
			}	
		}
	}
	///regardless of skinning state
	Object3D::onDraw();
}

/// Called from SceneGraph "sceneUpdate"
void SubMesh::sceneUpdate(D32 sceneTime){
	// update possible animation
	_renderSkeleton = false;
	if(GFX_DEVICE.getRenderStage() == FINAL_STAGE){
		if (_animator != NULL && getParentMesh()->_playAnimations) {
			///sceneTime is in miliseconds. Convert to seconds
			 _deltaTime = sceneTime/1000.0f;
			  //set the bone animation to the specified timestamp
			 _transforms = _animator->GetTransforms(_deltaTime);
			 if(!_transforms.empty()){
				 getParentMesh()->_updateBB = true;
			 }
			_renderSkeleton = true;
		}
	}
}

std::vector<mat4<F32> >& SubMesh::GetTransforms(){ 
	return _transforms; 
}

void SubMesh::renderSkeleton(){
	// update possible animation
	//assert(_animator != NULL);
	if(_renderSkeleton) {
		_animator->RenderSkeleton(_deltaTime);
	}
}

/// Create a mesh animator from assimp imported data
bool SubMesh::createAnimatorFromScene(const aiScene* scene,U8 subMeshPointer){
	assert(scene != NULL);
	/// Delete old animator if any
	SAFE_DELETE(_animator);
	if(scene->HasAnimations()){
		_animator = New SceneAnimator();
		_animator->Init(scene,subMeshPointer);
		_hasAnimations = true;
	}
	return true;
}
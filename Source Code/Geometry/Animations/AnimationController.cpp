#include "Headers/AnimationController.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"

#include <assimp/scene.h>

namespace Divide {

void SceneAnimator::Release(){// this should clean everything up
	for (LineCollection::value_type& it : _skeletonLines) {
		for (LineMap::value_type& it2 : it.second) {
            it2.second.clear();
        }
    }
    _skeletonLines.clear();
    _animations.clear();// clear all animations
    _bonesByName.clear();
    _bonesToIndex.clear();
    _animationNameToId.clear();
    _transforms.clear();
    // This node will delete all children recursivly
    MemoryManager::SAFE_DELETE( _skeleton );
}

bool SceneAnimator::Init(const aiScene* pScene, U8 meshPointer){// this will build the skeleton based on the scene passed to it and CLEAR EVERYTHING
    if(!pScene->HasAnimations()) return false;
    Release();

    _skeleton = CreateBoneTree( pScene->mRootNode, nullptr);
    ExtractAnimations(pScene);

    const aiMesh* mesh = pScene->mMeshes[meshPointer];

    for (U32 n = 0; n < mesh->mNumBones;++n){
        const aiBone* bone = mesh->mBones[n];
        hashMapImpl<stringImpl, Bone*>::iterator found = _bonesByName.find(bone->mName.data);
        if(found != _bonesByName.end()){// FOUND IT!!! woohoo, make sure its not already in the bone list
            bool skip = false;
            for(vectorAlg::vecSize j(0); j< _bones.size(); j++){
                stringImpl bname = bone->mName.data;
                if(_bones[j]->_name == bname) {
                    skip = true;// already inserted, skip this so as not to insert the same bone multiple times
                    break;
                }
            }
            if(!skip){// only insert the bone if it has not already been inserted
                stringImpl tes = found->second->_name;
                found->second->_offsetMatrix =  bone->mOffsetMatrix;
                _bones.push_back(found->second);
                _bonesToIndex[found->first] = (U32)_bones.size()-1;
            }
        }
    }

    _transforms.resize( _bones.size());
    D32 timestep = 1.0/ANIMATION_TICKS_PER_SECOND;// ANIMATION_TICKS_PER_SECOND
    mat4<F32> rotationmat;
    vectorImpl<mat4<F32> > vec;
    for(vectorAlg::vecSize i(0); i< _animations.size(); i++){// pre calculate the animations
        D32 dt = 0;
        for(D32 ticks = 0; ticks < _animations[i]._duration; ticks += _animations[i]._ticksPerSecond/ANIMATION_TICKS_PER_SECOND){
            dt +=timestep;
            Calculate((I32)i, dt);
            _animations[i]._transforms.push_back(vec);
            vectorImpl<mat4<F32> >& trans = _animations[i]._transforms.back();
            if(GFX_DEVICE.getApi() == Direct3D){
                for( vectorAlg::vecSize a = 0; a < _transforms.size(); ++a){
                    AnimUtils::TransformMatrix(_bones[a]->_offsetMatrix * _bones[a]->_globalTransform, rotationmat);
                    trans.push_back(rotationmat);
                }
            }else{
                for( vectorAlg::vecSize a = 0; a < _transforms.size(); ++a){
                    AnimUtils::TransformMatrix(_bones[a]->_globalTransform * _bones[a]->_offsetMatrix, rotationmat);
                    trans.push_back(rotationmat);
                }
            }
        }
    }

    D_PRINT_FN(Locale::get("LOAD_ANIMATIONS_END"), _bones.size());
    return !_transforms.empty();
}

void SceneAnimator::ExtractAnimations(const aiScene* pScene){
    D_PRINT_FN(Locale::get("LOAD_ANIMATIONS_BEGIN"));
    for(size_t i(0); i< pScene->mNumAnimations; i++){
        if(pScene->mAnimations[i]->mDuration > 0.0f){
            _animations.push_back(AnimEvaluator(pScene->mAnimations[i]) );// add the animations
        }
    }
    // get all the animation names so I can reference them by name and get the correct id
    U16 i = 0;
    for(AnimEvaluator& animation : _animations){
        _animationNameToId.insert(hashMapImpl<stringImpl, U32>::value_type(animation._name, i++));
    }
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::Calculate(I32 animationIndex, const D32 pTime){
    if ((animationIndex < 0) || (animationIndex >= (I32)_animations.size())) return;// invalid animation
    _animations[animationIndex].Evaluate(pTime, _bonesByName);
    UpdateTransforms(_skeleton);
}

// ------------------------------------------------------------------------------------------------
// Recursively creates an internal node structure matching the current scene and animation.
Bone* SceneAnimator::CreateBoneTree( aiNode* pNode, Bone* parent){
    Bone* internalNode = New Bone(pNode->mName.data);// create a node
    internalNode->_parent = parent; //set the parent, in the case this is theroot node, it will be null
    _bonesByName[internalNode->_name] = internalNode;// use the name as a key
    internalNode->_localTransform =  pNode->mTransformation;
    if(GFX_DEVICE.getApi() == Direct3D){
        internalNode->_localTransform.Transpose();
    }
    internalNode->_originalLocalTransform = internalNode->_localTransform;// a copy saved
    CalculateBoneToWorldTransform(internalNode);

    // continue for all child nodes and assign the created internal nodes as our children
    // recursively call this function on all children
    for(uint32_t i = 0; i < pNode->mNumChildren; i++){
        Bone* childNode = CreateBoneTree(pNode->mChildren[i], internalNode);
        internalNode->_children.push_back(childNode);
    }
    return internalNode;
}

/// ------------------------------------------------------------------------------------------------
/// Recursively updates the internal node transformations from the given matrix array
void SceneAnimator::UpdateTransforms(Bone* pNode) {
    CalculateBoneToWorldTransform( pNode);// update global transform as well
    /// continue for all children
    for(Bone* bone : pNode->_children){
        UpdateTransforms( bone );
    }
}

Bone* SceneAnimator::GetBoneByName(const stringImpl& bname) const {
    hashMapImpl<stringImpl, Bone*>::const_iterator found = _bonesByName.find(bname);
	if ( found != _bonesByName.end() ) {
		return found->second;
	} else {
		return nullptr;
	}
}

I32 SceneAnimator::GetBoneIndex(const stringImpl& bname) const {
    hashMapImpl<stringImpl, U32>::const_iterator found = _bonesToIndex.find(bname);
	if ( found != _bonesToIndex.end() ) {
		return found->second;
	} else {
		return -1;
	}
}

/// ------------------------------------------------------------------------------------------------
/// Calculates the global transformation matrix for the given internal node
void SceneAnimator::CalculateBoneToWorldTransform(Bone* child){
    child->_globalTransform = child->_localTransform;
    Bone* parent = child->_parent;
    // this will climb the nodes up along through the parents concatenating all the matrices to get the Object to World transform, or in this case, the Bone To World transform
    if(GFX_DEVICE.getApi() == Direct3D){
        while( parent ){
            child->_globalTransform *= parent->_localTransform;
            parent  = parent->_parent;// get the parent of the bone we are working on
        }
    }else{
        while( parent) {
            child->_globalTransform = parent->_localTransform * child->_globalTransform;
            parent  = parent->_parent;// get the parent of the bone we are working on
        }
    }
}

///Renders the current skeleton pose at time index dt
const vectorImpl<Line >& SceneAnimator::getSkeletonLines(U32 animationIndex, const D32 dt) {
    I32 frameIndex = _animations[animationIndex].GetFrameIndexAt(dt);
    LineCollection::iterator it = _skeletonLines.find(animationIndex);
    if (it == _skeletonLines.end()) {
        hashAlg::pair<LineCollection::iterator, bool> result = hashAlg::insert(_skeletonLines, hashAlg::makePair(animationIndex, LineMap()));
        assert(result.second);
        it = result.first;
    }

    // create all the needed points
    vectorImpl<Line >& lines = it->second[frameIndex];
    if (lines.empty()){
        lines.reserve(_bones.size());
        // Construct skeleton
        Calculate(animationIndex, dt);
        // Start with identity transform
        CreateSkeleton(_skeleton, aiMatrix4x4(), lines);
    }

    return lines;
}

/// Create animation skeleton
I32 SceneAnimator::CreateSkeleton(Bone* piNode, const aiMatrix4x4& parent, vectorImpl<Line >& lines){

    aiMatrix4x4 me = piNode->_globalTransform;
    if(GFX_DEVICE.getApi() == Direct3D){
        me.Transpose();
    }

    if (piNode->_parent) {
        lines.push_back(Line(vec3<F32>(parent.a4, parent.b4, parent.c4),
                             vec3<F32>(me.a4, me.b4, me.c4),
                             vec4<U8>(255,0,0,255)));
    }

    // render all child nodes
    for(Bone* bone : piNode->_children){
        CreateSkeleton(bone, me, lines);
    }

    return 1;
}

};
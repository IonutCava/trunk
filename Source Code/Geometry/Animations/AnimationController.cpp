#include "Headers/AnimationController.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"
using namespace boost;

void SceneAnimator::Release(){// this should clean everything up
    _currentAnimIndex = -1;
    for_each(pointCollection::value_type& it, _pointsA){
        for_each(pointMap::value_type& it2, it.second){
            it2.second.clear();
        }
    }

    for_each(pointCollection::value_type& it, _pointsB){
        for_each(pointMap::value_type& it2, it.second){
            it2.second.clear();
        }
     }

    for_each(colorCollection::value_type& it, _colors){
        for_each(colorMap::value_type& it2, it.second){
            it2.second.clear();
        }
     }

    _pointsA.clear();
    _pointsB.clear();
    _colors.clear();
    _animations.clear();// clear all animations
    _bonesByName.clear();
    _bonesToIndex.clear();
    _animationNameToId.clear();
    _transforms.clear();
    // This node will delete all children recursivly
    SAFE_DELETE(_skeleton);
}

void SceneAnimator::Init(const aiScene* pScene, U8 meshPointer){// this will build the skeleton based on the scene passed to it and CLEAR EVERYTHING
    if(!pScene->HasAnimations()) return;
    Release();

    _skeleton = CreateBoneTree( pScene->mRootNode, NULL);
    ExtractAnimations(pScene);

    const aiMesh* mesh = pScene->mMeshes[meshPointer];

    for (U32 n = 0; n < mesh->mNumBones;++n){
        const aiBone* bone = mesh->mBones[n];
        Unordered_map<std::string, Bone*>::iterator found = _bonesByName.find(bone->mName.data);
        if(found != _bonesByName.end()){// FOUND IT!!! woohoo, make sure its not already in the bone list
            bool skip = false;
            for(size_t j(0); j< _bones.size(); j++){
                std::string bname = bone->mName.data;
                if(_bones[j]->_name == bname) {
                    skip = true;// already inserted, skip this so as not to insert the same bone multiple times
                    break;
                }
            }
            if(!skip){// only insert the bone if it has not already been inserted
                std::string tes = found->second->_name;
                found->second->_offsetMatrix =  bone->mOffsetMatrix;
                _bones.push_back(found->second);
                _bonesToIndex[found->first] = _bones.size()-1;
            }
        }
    }

    _transforms.resize( _bones.size());
    D32 timestep = 1.0/ANIMATION_TICKS_PER_SECOND;// ANIMATION_TICKS_PER_SECOND
    mat4<F32> rotationmat;
    vectorImpl<mat4<F32> > vec;
    for(size_t i(0); i< _animations.size(); i++){// pre calculate the animations
        SetAnimIndex(i);
        D32 dt = 0;
        for(D32 ticks = 0; ticks < _animations[i]._duration; ticks += _animations[i]._ticksPerSecond/ANIMATION_TICKS_PER_SECOND){
            dt +=timestep;
            Calculate(dt);
            _animations[i]._transforms.push_back(vec);
            vectorImpl<mat4<F32> >& trans = _animations[i]._transforms.back();
            if(GFX_DEVICE.getApi() == Direct3D){
                for( size_t a = 0; a < _transforms.size(); ++a){
                    AnimUtils::TransformMatrix(rotationmat, _bones[a]->_offsetMatrix * _bones[a]->_globalTransform);
                    trans.push_back(rotationmat);
                }
            }else{
                for( size_t a = 0; a < _transforms.size(); ++a){
                    AnimUtils::TransformMatrix(rotationmat, _bones[a]->_globalTransform * _bones[a]->_offsetMatrix);
                    trans.push_back(rotationmat);
                }
            }
        }
    }

    D_PRINT_FN(Locale::get("LOAD_ANIMATIONS_END"), _bones.size());
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
    for_each(AnimEvaluator& animation, _animations){
        _animationNameToId.insert(Unordered_map<std::string, U32>::value_type(animation._name, i++));
    }
    _currentAnimIndex=0;
    SetAnimation("Idle");
}

bool SceneAnimator::SetAnimation(const std::string& name){
    Unordered_map<std::string, U32>::iterator itr = _animationNameToId.find(name);
    I32 oldindex = _currentAnimIndex;
    if(itr !=_animationNameToId.end()) _currentAnimIndex = itr->second;
    return oldindex != _currentAnimIndex;
}

bool SceneAnimator::SetAnimIndex( I32  pAnimIndex){
    if(pAnimIndex >= (I32)_animations.size()) return false;// no change, or the animations data is out of bounds
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = pAnimIndex;// only set this after the checks for good data and the object was actually inserted
    return oldindex != _currentAnimIndex;
}

bool SceneAnimator::SetNextAnimation() {
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = ++_currentAnimIndex % _animations.size();
    return oldindex != _currentAnimIndex;
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::Calculate(const D32 pTime){
    if( (_currentAnimIndex < 0) || (_currentAnimIndex >= (I32)_animations.size()) ) return;// invalid animation
    _animations[_currentAnimIndex].Evaluate( pTime, _bonesByName);
    UpdateTransforms(_skeleton);
}

// ------------------------------------------------------------------------------------------------
// Recursively creates an internal node structure matching the current scene and animation.
Bone* SceneAnimator::CreateBoneTree( aiNode* pNode, Bone* parent){
    Bone* internalNode = new Bone(pNode->mName.data);// create a node
    internalNode->_parent = parent; //set the parent, in the case this is theroot node, it will be null
    _bonesByName[internalNode->_name] = internalNode;// use the name as a key
    internalNode->_localTransform =  pNode->mTransformation;
    if(GFX_DEVICE.getApi() == Direct3D){
        internalNode->_localTransform.Transpose();
    }
    internalNode->_originalLocalTransform = internalNode->_localTransform;// a copy saved
    CalculateBoneToWorldTransform(internalNode);

    // continue for all child nodes and assign the created internal nodes as our children
    // recursivly call this function on all children
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
    for_each(Bone* bone, pNode->_children){
        UpdateTransforms( bone );
    }
}

Bone* SceneAnimator::GetBoneByName(const std::string& bname) const {
    Unordered_map<std::string, Bone*>::const_iterator found = _bonesByName.find(bname);
	if(found != _bonesByName.end()) 
		return found->second; 
	else 
		return NULL;
}

I32 SceneAnimator::GetBoneIndex(const std::string& bname) const {
	Unordered_map<std::string, U32>::const_iterator found = _bonesToIndex.find(bname);
	if(found != _bonesToIndex.end()) 
		return found->second; 
	else 
		return -1;
}

/// ------------------------------------------------------------------------------------------------
/// Calculates the global transformation matrix for the given internal node
void SceneAnimator::CalculateBoneToWorldTransform(Bone* child){
    child->_globalTransform = child->_localTransform;
    Bone* parent = child->_parent;
    // this will climb the nodes up along through the parents concentating all the matrices to get the Object to World transform, or in this case, the Bone To World transform
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
I32 SceneAnimator::RenderSkeleton(const D32 dt){
    U32 frameIndex = _animations[_currentAnimIndex].GetFrameIndexAt(dt);

    if(_pointsA.find(_currentAnimIndex) == _pointsA.end()){
        pointMap pointsA;
        pointMap pointsB;
        colorMap colors;
        _pointsA.insert(std::make_pair(_currentAnimIndex, pointsA));
        _pointsB.insert(std::make_pair(_currentAnimIndex, pointsB));
        _colors.insert(std::make_pair(_currentAnimIndex, colors));
    }

    if(_pointsA[_currentAnimIndex][frameIndex].empty()){
        // create all the needed points
        vectorImpl<vec3<F32> >& pA = _pointsA[_currentAnimIndex][frameIndex];
        vectorImpl<vec3<F32> >& pB = _pointsB[_currentAnimIndex][frameIndex];
        vectorImpl<vec4<U8> >& cl = _colors[_currentAnimIndex][frameIndex];
        pA.reserve(_bones.size());
        pB.reserve(_bones.size());
        cl.reserve(_bones.size());
        // Construct skeleton
        Calculate(dt);
        // Prepare global transform
        aiMatrix4x4 rootTransform;
        AnimUtils::TransformMatrix(rootTransform,_rootTransformRender);
        CreateSkeleton(_skeleton, rootTransform, pA, pB, cl);
    }
    // Submit skeleton to gpu
    return SubmitSkeletonToGPU(frameIndex);
}

/// Create animation skeleton
I32 SceneAnimator::CreateSkeleton(Bone* piNode, const aiMatrix4x4& parent, vectorImpl<vec3<F32> >& pointsA,
                                                                           vectorImpl<vec3<F32> >& pointsB,
                                                                           vectorImpl<vec4<U8> >& colors){
    aiMatrix4x4 me = piNode->_globalTransform;
    if(GFX_DEVICE.getApi() == Direct3D){
        me.Transpose();
    }

    if (piNode->_parent) {
        colors.push_back(vec4<U8>(255,0,0,255));
        pointsA.push_back(vec3<F32>(parent.a4, parent.b4, parent.c4));
        pointsB.push_back(vec3<F32>(me.a4, me.b4, me.c4));
    }

    // render all child nodes
    for_each(Bone* bone, piNode->_children){
        CreateSkeleton(bone, me, pointsA, pointsB, colors);
    }

    return 1;
}

I32 SceneAnimator::SubmitSkeletonToGPU(U32 frameIndex){
    GFX_DEVICE.drawLines(_pointsA[_currentAnimIndex][frameIndex],
                         _pointsB[_currentAnimIndex][frameIndex],
                         _colors[_currentAnimIndex][frameIndex],
                         _rootTransformRender,
                         false,
                         true);
    return 1;
}
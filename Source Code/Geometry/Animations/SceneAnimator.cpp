#include "Headers/SceneAnimator.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include <assimp/scene.h>

namespace Divide {

void SceneAnimator::release() {
    // this should clean everything up
    _skeletonLines.clear();
    _skeletonLinesContainer.clear();
    // clear all animations
    _animations.clear();
    _bonesByName.clear();
    _bonesToIndex.clear();
    _animationNameToID.clear();
    _transforms.clear();
    // This node will delete all children recursivly
    MemoryManager::DELETE(_skeleton);
}

/// This will build the skeleton based on the scene passed to it and CLEAR
/// EVERYTHING
bool SceneAnimator::init(const aiScene* pScene, U8 meshPointer) {
    if (!pScene->HasAnimations()) {
        return false;
    }

    release();

    _skeleton = createBoneTree(pScene->mRootNode, nullptr);
    extractAnimations(pScene);

    const aiMesh* mesh = pScene->mMeshes[meshPointer];

    for (U32 n = 0; n < mesh->mNumBones; ++n) {
        const aiBone* bone = mesh->mBones[n];
        hashMapImpl<stringImpl, Bone*>::iterator found =
            _bonesByName.find(bone->mName.data);
        if (found != std::end(_bonesByName)) {  // FOUND IT!!! woohoo, make sure
                                                // its not already in the bone
                                                // list
            bool skip = false;
            for (vectorAlg::vecSize j(0); j < _bones.size(); j++) {
                stringImpl bname = bone->mName.data;
                if (_bones[j]->_name == bname) {
                    skip = true;  // already inserted, skip this so as not to
                                  // insert the same bone multiple times
                    break;
                }
            }
            if (!skip) {  // only insert the bone if it has not already been
                          // inserted
                stringImpl tes = found->second->_name;
                found->second->_offsetMatrix = bone->mOffsetMatrix;
                _bones.push_back(found->second);
                _bonesToIndex[found->first] = (U32)_bones.size() - 1;
            }
        }
    }

    _transforms.resize(_bones.size());
    D32 timestep =
        1.0 / ANIMATION_TICKS_PER_SECOND;  // ANIMATION_TICKS_PER_SECOND
    mat4<F32> rotationmat;
    vectorImpl<mat4<F32> > vec;
    for (vectorAlg::vecSize i(0); i < _animations.size();
         i++) {  // pre calculate the animations
        D32 dt = 0;
        for (D32 ticks = 0; ticks < _animations[i].duration();
             ticks +=
             _animations[i].ticksPerSecond() / ANIMATION_TICKS_PER_SECOND) {
            dt += timestep;
            calculate((I32)i, dt);
            _animations[i].transforms().push_back(vec);
            vectorImpl<mat4<F32> >& trans = _animations[i].transforms().back();
            if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
                for (vectorAlg::vecSize a = 0; a < _transforms.size(); ++a) {
                    AnimUtils::TransformMatrix(
                        _bones[a]->_offsetMatrix * _bones[a]->_globalTransform,
                        rotationmat);
                    trans.push_back(rotationmat);
                }
            } else {
                for (vectorAlg::vecSize a = 0; a < _transforms.size(); ++a) {
                    AnimUtils::TransformMatrix(
                        _bones[a]->_globalTransform * _bones[a]->_offsetMatrix,
                        rotationmat);
                    trans.push_back(rotationmat);
                }
            }
        }
    }

    Console::d_printfn(Locale::get("LOAD_ANIMATIONS_END"), _bones.size());
    return !_transforms.empty();
}

void SceneAnimator::extractAnimations(const aiScene* pScene) {
    Console::d_printfn(Locale::get("LOAD_ANIMATIONS_BEGIN"));
    for (size_t i(0); i < pScene->mNumAnimations; i++) {
        if (pScene->mAnimations[i]->mDuration > 0.0f) {
            _animations.push_back(
                AnimEvaluator(pScene->mAnimations[i]));  // add the animations
        }
    }
    // get all the animation names so I can reference them by name and get the
    // correct id
    U16 i = 0;
    for (AnimEvaluator& animation : _animations) {
        _animationNameToID.insert(
            hashMapImpl<stringImpl, U32>::value_type(animation.name(), i++));
    }
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::calculate(I32 animationIndex, const D32 pTime) {
    if ((animationIndex < 0) || (animationIndex >= (I32)_animations.size()))
        return;  // invalid animation
    _animations[animationIndex].evaluate(pTime, _bonesByName);
    updateTransforms(_skeleton);
}

// ------------------------------------------------------------------------------------------------
// Recursively creates an internal node structure matching the current scene and
// animation.
Bone* SceneAnimator::createBoneTree(aiNode* pNode, Bone* parent) {
    Bone* internalNode =
        MemoryManager_NEW Bone(pNode->mName.data);  // create a node
    internalNode->_parent = parent;  // set the parent, in the case this is
                                     // theroot node, it will be null
    _bonesByName[internalNode->_name] = internalNode;  // use the name as a key
    internalNode->_localTransform = pNode->mTransformation;
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
        internalNode->_localTransform.Transpose();
    }
    internalNode->_originalLocalTransform =
        internalNode->_localTransform;  // a copy saved
    calculateBoneToWorldTransform(internalNode);

    // continue for all child nodes and assign the created internal nodes as our
    // children
    // recursively call this function on all children
    for (uint32_t i = 0; i < pNode->mNumChildren; i++) {
        Bone* childNode = createBoneTree(pNode->mChildren[i], internalNode);
        internalNode->_children.push_back(childNode);
    }
    return internalNode;
}

/// ------------------------------------------------------------------------------------------------
/// Recursively updates the internal node transformations from the given matrix
/// array
void SceneAnimator::updateTransforms(Bone* pNode) {
    calculateBoneToWorldTransform(pNode);  // update global transform as well
    /// continue for all children
    for (Bone* bone : pNode->_children) {
        updateTransforms(bone);
    }
}

Bone* SceneAnimator::boneByName(const stringImpl& bname) const {
    hashMapImpl<stringImpl, Bone*>::const_iterator found =
        _bonesByName.find(bname);
    if (found != std::end(_bonesByName)) {
        return found->second;
    } else {
        return nullptr;
    }
}

I32 SceneAnimator::boneIndex(const stringImpl& bname) const {
    hashMapImpl<stringImpl, U32>::const_iterator found =
        _bonesToIndex.find(bname);
    if (found != std::end(_bonesToIndex)) {
        return found->second;
    } else {
        return -1;
    }
}

/// ------------------------------------------------------------------------------------------------
/// Calculates the global transformation matrix for the given internal node
void SceneAnimator::calculateBoneToWorldTransform(Bone* child) {
    child->_globalTransform = child->_localTransform;
    Bone* parent = child->_parent;
    // This will climb the nodes up along through the parents concatenating all
    // the matrices to get the Object to World transform,
    // or in this case, the Bone To World transform
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
        while (parent) {
            child->_globalTransform *= parent->_localTransform;
            parent =
                parent
                    ->_parent;  // get the parent of the bone we are working on
        }
    } else {
        while (parent) {
            child->_globalTransform =
                parent->_localTransform * child->_globalTransform;
            parent =
                parent
                    ->_parent;  // get the parent of the bone we are working on
        }
    }
}

/// Renders the current skeleton pose at time index dt
const vectorImpl<Line>& SceneAnimator::skeletonLines(I32 animationIndex,
                                                        const D32 dt) {
    I32 frameIndex = _animations[animationIndex].frameIndexAt(dt);
    LineMap& lineMap = _skeletonLines[animationIndex];
    LineMap::iterator it = lineMap.find(frameIndex);

    if (it == std::end(lineMap)) {
        vectorAlg::vecSize lineEntry =
            static_cast<vectorAlg::vecSize>(_skeletonLinesContainer.size());
        it = hashAlg::insert(lineMap,
                             hashAlg::makePairCpy(frameIndex, lineEntry)).first;

        _skeletonLinesContainer.push_back(vectorImpl<Line>());
    }

    // create all the needed points
    vectorImpl<Line>& lines = _skeletonLinesContainer[it->second];
    if (lines.empty()) {
        lines.reserve(_bones.size());
        // Construct skeleton
        calculate(animationIndex, dt);
        // Start with identity transform
        createSkeleton(_skeleton, aiMatrix4x4(), lines);
    }

    return lines;
}

/// Create animation skeleton
I32 SceneAnimator::createSkeleton(Bone* piNode, const aiMatrix4x4& parent,
                                  vectorImpl<Line>& lines) {

    const aiMatrix4x4& me = piNode->_globalTransform;

    vec3<F32> startPoint, endPoint;
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
        startPoint.set(parent.d1, parent.d2, parent.d3);
        endPoint.set(me.d1, me.d2, me.d3);
    } else {
        startPoint.set(parent.a4, parent.b4, parent.c4);
        endPoint.set(me.a4, me.b4, me.c4);
    }

    if (piNode->_parent) {
        lines.push_back(Line(startPoint, endPoint, vec4<U8>(255, 0, 0, 255)));
    }

    // render all child nodes
    for (Bone* bone : piNode->_children) {
        createSkeleton(bone, me, lines);
    }

    return 1;
}
};
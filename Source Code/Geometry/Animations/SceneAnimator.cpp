#include "Headers/SceneAnimator.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

#include <assimp/scene.h>

namespace Divide {

SceneAnimator::SceneAnimator()
    : _skeleton(nullptr),
    _skeletonDepthCache(-1)
{
}

SceneAnimator::~SceneAnimator()
{
    release();
}

void SceneAnimator::release()
{
    // this should clean everything up
    _skeletonLines.clear();
    _skeletonLinesContainer.clear();
    _skeletonDepthCache = -2;
    // clear all animations
    _animations.clear();
    _animationNameToID.clear();
    _transforms.clear();
    // This node will delete all children recursivly
    MemoryManager::DELETE(_skeleton);
}

/// This will build the skeleton based on the scene passed to it and CLEAR
/// EVERYTHING
bool SceneAnimator::init(const aiScene* pScene) {
    if (!pScene->HasAnimations()) {
        return false;
    }

    release();

    _skeleton = createBoneTree(pScene->mRootNode, nullptr);
    _skeletonDepthCache = to_int(_skeleton->hierarchyDepth());

    vectorImpl<Bone*> bones;
    bones.reserve(_skeletonDepthCache);
    
    for (U16 meshPointer = 0; meshPointer < pScene->mNumMeshes; ++meshPointer) {
        const aiMesh* mesh = pScene->mMeshes[meshPointer];
        for (U32 n = 0; n < mesh->mNumBones; ++n) {
            const aiBone* bone = mesh->mBones[n];

            Bone* found = _skeleton->find(bone->mName.data);
            assert(found != nullptr);

            found->_offsetMatrix = bone->mOffsetMatrix;
            bones.push_back(found);
        }
    }

    if (_skeletonDepthCache != to_int(bones.size())) {
        // ToDo: investigate mismatch. 
        // Avoid adding unneeded bones to skeleton - Ionut
        _skeletonDepthCache = to_int(bones.size());
    }

    extractAnimations(pScene);

    _transforms.resize(_skeletonDepthCache);

    D32 timestep = 1.0 / ANIMATION_TICKS_PER_SECOND;
    mat4<F32> rotationmat;
    vectorImpl<mat4<F32> > vec;
   
    // pre calculate the animations
    for (vectorAlg::vecSize i(0); i < _animations.size(); ++i) {
        AnimEvaluator& crtAnimation = _animations[i];
        D32 duration = crtAnimation.duration();
        D32 tickStep =
            crtAnimation.ticksPerSecond() / ANIMATION_TICKS_PER_SECOND;
        D32 dt = 0;
        for (D32 ticks = 0; ticks < duration; ticks += tickStep) {
            dt += timestep;
            calculate((I32)i, dt);
            crtAnimation.transforms().push_back(vec);
            vectorImpl<mat4<F32> >& trans = crtAnimation.transforms().back();
            if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
                for (I32 a = 0; a < _skeletonDepthCache; ++a) {
                    Bone* bone = bones[a];
                    AnimUtils::TransformMatrix(
                        bone->_offsetMatrix * bone->_globalTransform,
                        rotationmat);
                    trans.push_back(rotationmat);
                    bone->_boneID = a;
                }
            } else {
                for (I32 a = 0; a < _skeletonDepthCache; ++a) {
                    Bone* bone = bones[a];
                    AnimUtils::TransformMatrix(
                        bone->_globalTransform * bone->_offsetMatrix,
                        rotationmat);
                    trans.push_back(rotationmat);
                    bone->_boneID = a;
                }
            }
        }
    }

    Console::d_printfn(Locale::get("LOAD_ANIMATIONS_END"), _skeletonDepthCache);

    return !_transforms.empty();
}

void SceneAnimator::extractAnimations(const aiScene* pScene) {
    Console::d_printfn(Locale::get("LOAD_ANIMATIONS_BEGIN"));
    for (size_t i(0); i < pScene->mNumAnimations; i++) {
        if (pScene->mAnimations[i]->mDuration > 0.0f) {
            // add the animations
            _animations.push_back(AnimEvaluator(pScene->mAnimations[i]));  
        }
    }
    // get all the animation names so I can reference them by name and get the
    // correct id
    U16 i = 0;
    for (AnimEvaluator& animation : _animations) {
        _animationNameToID.insert(hashAlg::makePair(animation.name(), i++));
    }
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::calculate(I32 animationIndex, const D32 pTime) {
    assert(_skeleton != nullptr);

    if ((animationIndex < 0) || (animationIndex >= (I32)_animations.size())) {
        return;  // invalid animation
    }
    _animations[animationIndex].evaluate(pTime, _skeleton);
    updateTransforms(_skeleton);
}

// ------------------------------------------------------------------------------------------------
// Recursively creates an internal node structure matching the current scene and
// animation.
Bone* SceneAnimator::createBoneTree(aiNode* pNode, Bone* parent) {
    Bone* internalNode =  MemoryManager_NEW Bone(pNode->mName.data); 
    // set the parent; in case this is the root node, it will be null
    internalNode->_parent = parent;  

    internalNode->_localTransform = pNode->mTransformation;
    if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
        internalNode->_localTransform.Transpose();
    }

    internalNode->_originalLocalTransform = internalNode->_localTransform;
    calculateBoneToWorldTransform(internalNode);

    // continue for all child nodes and assign the created internal nodes as our
    // children recursively call this function on all children
    for (U32 i = 0; i < pNode->mNumChildren; ++i) {
        internalNode->_children.push_back(
            createBoneTree(pNode->mChildren[i], internalNode));
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
    assert(_skeleton != nullptr);

    return _skeleton->find(bname);
}

I32 SceneAnimator::boneIndex(const stringImpl& bname) const {
    Bone* bone = boneByName(bname);

    if (bone != nullptr) {
        return bone->_boneID;
    }

    return -1;
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
            // get the parent of the bone we are working on
            parent = parent->_parent;
        }
    } else {
        while (parent) {
            child->_globalTransform =
                parent->_localTransform * child->_globalTransform;
            // get the parent of the bone we are working on
            parent = parent->_parent;
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
        lines.reserve(boneCount());
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
#include "stdafx.h"

#include "config.h"

#include "Headers/SceneAnimator.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"

#include "Core/Headers/PlatformContext.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

/// ------------------------------------------------------------------------------------------------
/// Calculates the global transformation matrix for the given internal node
void calculateBoneToWorldTransform(Bone* child) {
    child->_globalTransform = child->_localTransform;
    Bone* parent = child->_parent;
    // This will climb the nodes up along through the parents concatenating all
    // the matrices to get the Object to World transform,
    // or in this case, the Bone To World transform
    while (parent) {
        child->_globalTransform *= parent->_localTransform;
        // get the parent of the bone we are working on
        parent = parent->_parent;
    }
}

SceneAnimator::SceneAnimator()
    : _skeleton(nullptr),
    _skeletonDepthCache(-1),
    _maximumAnimationFrames(0)
{
}

SceneAnimator::~SceneAnimator()
{
    release();
}

void SceneAnimator::release(bool releaseAnimations)
{
    // this should clean everything up
    _skeletonLines.clear();
    _skeletonLinesContainer.clear();
    _skeletonDepthCache = -2;
    if (releaseAnimations) {
        // clear all animations
        _animations.clear();
        _animationNameToID.clear();
        _transforms.clear();
    }
    // This node will delete all children recursively
    MemoryManager::SAFE_DELETE(_skeleton);
}

bool SceneAnimator::init(PlatformContext& context) {
    Console::d_printfn(Locale::get(_ID("LOAD_ANIMATIONS_BEGIN")));

    _transforms.resize(_skeletonDepthCache);

    const D64 timestep = 1.0 / ANIMATION_TICKS_PER_SECOND;
    vectorEASTL<mat4<F32>> vec;
    const size_t animationCount = _animations.size();
    _skeletonLines.resize(animationCount);

    // pre-calculate the animations
    for (size_t i = 0; i < animationCount; ++i) {
        const std::shared_ptr<AnimEvaluator>& crtAnimation = _animations[i];
        const D64 duration = crtAnimation->duration();
        const D64 tickStep = crtAnimation->ticksPerSecond() / ANIMATION_TICKS_PER_SECOND;
        D64 dt = 0;
        for (D64 ticks = 0; ticks < duration; ticks += tickStep) {
            dt += timestep;
            calculate((I32)i, dt);
            vec.resize(_skeletonDepthCache, MAT4_IDENTITY);
            for (I32 a = 0; a < _skeletonDepthCache; ++a) {
                Bone* bone = _bones[a];
                vec[a] = bone->_offsetMatrix * bone->_globalTransform;
                bone->_boneID = a;
            }
            crtAnimation->transforms().push_back(vec);
        }

        _maximumAnimationFrames = std::max(crtAnimation->frameCount(), _maximumAnimationFrames);
        _skeletonLines[i].resize(_maximumAnimationFrames, -1);
    }

    // pay the cost upfront
    for(const std::shared_ptr<AnimEvaluator>& crtAnimation : _animations) {
        crtAnimation->initBuffers(context.gfx());
    }

     Console::d_printfn(Locale::get(_ID("LOAD_ANIMATIONS_END")), _skeletonDepthCache);

    return !_transforms.empty();
}

/// This will build the skeleton based on the scene passed to it and CLEAR EVERYTHING
bool SceneAnimator::init(PlatformContext& context, Bone* const skeleton, const vectorEASTL<Bone*>& bones) {
    release(false);
    _skeleton = skeleton;
    _bones = bones;
    _skeletonDepthCache = to_I32(_bones.size());
    return init(context);
   
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::calculate(I32 animationIndex, const D64 pTime) {
    assert(_skeleton != nullptr);

    if ((animationIndex < 0) || (animationIndex >= (I32)_animations.size())) {
        return;  // invalid animation
    }
    _animations[animationIndex]->evaluate(pTime, _skeleton);
    updateTransforms(_skeleton);
}

/// ------------------------------------------------------------------------------------------------
/// Recursively updates the internal node transformations from the given matrix array
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

/// Renders the current skeleton pose at time index dt
const vectorEASTL<Line>& SceneAnimator::skeletonLines(I32 animationIndex, const D64 dt) {
    const I32 frameIndex = std::max(_animations[animationIndex]->frameIndexAt(dt) - 1, 0);
    I32& vecIndex = _skeletonLines.at(animationIndex).at(frameIndex);

    if (vecIndex == -1) {
        vecIndex = to_I32(_skeletonLinesContainer.size());
        _skeletonLinesContainer.emplace_back();
    }

    // create all the needed points
    vectorEASTL<Line>& lines = _skeletonLinesContainer.at(vecIndex);
    if (lines.empty()) {
        lines.reserve(boneCount());
        // Construct skeleton
        calculate(animationIndex, dt);
        // Start with identity transform
        createSkeleton(_skeleton, MAT4_IDENTITY, lines);
    }

    return lines;
}

/// Create animation skeleton
I32 SceneAnimator::createSkeleton(Bone* piNode,
                                  const mat4<F32>& parent,
                                  vectorEASTL<Line>& lines) {
    static Line s_line = { VECTOR3_ZERO, VECTOR3_UNIT, DefaultColours::RED_U8, DefaultColours::RED_U8, 2.0f, 2.0f };
    const mat4<F32>& me = piNode->_globalTransform;

    if (piNode->_parent) {
        Line& line = lines.emplace_back(s_line);
        line.positionStart(parent.getRow(3).xyz());
        line.positionEnd(me.getRow(3).xyz());
    }

    // render all child nodes
    for (Bone* bone : piNode->_children) {
        createSkeleton(bone, me, lines);
    }

    return 1;
}
};
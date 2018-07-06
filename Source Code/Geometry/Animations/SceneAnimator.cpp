#include "Headers/SceneAnimator.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Headers/AnimationUtils.h"

#include "Core/Headers/Console.h"
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
    if (Config::USE_OPENGL_RENDERING == true) {
        while (parent) {
            child->_globalTransform =  parent->_localTransform * child->_globalTransform;
            // get the parent of the bone we are working on
            parent = parent->_parent;
        }
    } else {
        while (parent) {
            child->_globalTransform *= parent->_localTransform;
            // get the parent of the bone we are working on
            parent = parent->_parent;
        }
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
    MemoryManager::DELETE(_skeleton);
}

bool SceneAnimator::init() {
    Console::d_printfn(Locale::get("LOAD_ANIMATIONS_BEGIN"));

    _transforms.resize(_skeletonDepthCache);

    D32 timestep = 1.0 / ANIMATION_TICKS_PER_SECOND;
    mat4<F32> rotationmat;
    vectorImpl<mat4<F32> > vec;

    // pre-calculate the animations
    for (vectorAlg::vecSize i(0); i < _animations.size(); ++i) {
        std::shared_ptr<AnimEvaluator> crtAnimation = _animations[i];
        D32 duration = crtAnimation->duration();
        D32 tickStep = crtAnimation->ticksPerSecond() / ANIMATION_TICKS_PER_SECOND;
        D32 dt = 0;
        for (D32 ticks = 0; ticks < duration; ticks += tickStep) {
            dt += timestep;
            calculate((I32)i, dt);
            crtAnimation->transforms().push_back(vec);
            vectorImpl<mat4<F32> >& trans = crtAnimation->transforms().back();
            if (Config::USE_OPENGL_RENDERING) {
                for (I32 a = 0; a < _skeletonDepthCache; ++a) {
                    Bone* bone = _bones[a];
                    AnimUtils::TransformMatrix(bone->_globalTransform * bone->_offsetMatrix, rotationmat);
                    trans.push_back(rotationmat);
                    bone->_boneID = a;
                }
            } else {
                for (I32 a = 0; a < _skeletonDepthCache; ++a) {
                    Bone* bone = _bones[a];
                    AnimUtils::TransformMatrix(bone->_offsetMatrix * bone->_globalTransform, rotationmat);
                    trans.push_back(rotationmat);
                    bone->_boneID = a;
                }
            }
        }
        _maximumAnimationFrames = std::max(crtAnimation->frameCount(), _maximumAnimationFrames);
    }

     Console::d_printfn(Locale::get("LOAD_ANIMATIONS_END"), _skeletonDepthCache);

    return !_transforms.empty();
}

/// This will build the skeleton based on the scene passed to it and CLEAR EVERYTHING
bool SceneAnimator::init(Bone* skeleton, const vectorImpl<Bone*>& bones) {
    release(false);
    _skeleton = skeleton;
    _bones = bones;
    _skeletonDepthCache = to_int(_bones.size());
    return init();
   
}

void SceneAnimator::registerAnimation(std::shared_ptr<AnimEvaluator> animation) {
    hashAlg::emplace(_animationNameToID, animation->name(), to_uint(_animations.size()));
    _animations.push_back(animation);
}

// ------------------------------------------------------------------------------------------------
// Calculates the node transformations for the scene.
void SceneAnimator::calculate(I32 animationIndex, const D32 pTime) {
    assert(_skeleton != nullptr);

    if ((animationIndex < 0) || (animationIndex >= (I32)_animations.size())) {
        return;  // invalid animation
    }
    _animations[animationIndex]->evaluate(pTime, _skeleton);
    updateTransforms(_skeleton);
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

/// Renders the current skeleton pose at time index dt
const vectorImpl<Line>& SceneAnimator::skeletonLines(I32 animationIndex,
                                                     const D32 dt) {
    I32 frameIndex = _animations[animationIndex]->frameIndexAt(dt);
    LineMap& lineMap = _skeletonLines[animationIndex];
    LineMap::iterator it = lineMap.find(std::max(frameIndex - 2, 0));

    if (it == std::end(lineMap)) {
        vectorAlg::vecSize lineEntry =
            static_cast<vectorAlg::vecSize>(_skeletonLinesContainer.size());
        it = hashAlg::insert(lineMap,
                             std::make_pair(frameIndex, lineEntry)).first;

        _skeletonLinesContainer.push_back(vectorImpl<Line>());
    }

    // create all the needed points
    vectorImpl<Line>& lines = _skeletonLinesContainer[it->second];
    if (lines.empty()) {
        lines.reserve(vectorAlg::vecSize(boneCount()));
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
        lines.push_back(Line(startPoint, endPoint, vec4<U8>(255, 0, 0, 255), 2.0f));
    }

    // render all child nodes
    for (Bone* bone : piNode->_children) {
        createSkeleton(bone, me, lines);
    }

    return 1;
}
};
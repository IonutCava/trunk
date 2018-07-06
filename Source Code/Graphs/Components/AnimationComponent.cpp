#include "Headers/AnimationComponent.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

AnimationComponent::AnimationComponent(SceneAnimator& animator,
                                       SceneGraphNode& parentSGN)

    : SGNComponent(SGNComponent::ComponentType::ANIMATION, parentSGN),
      _animator(animator),
      _playAnimations(true),
      _currentTimeStamp(0UL),
      _parentTimeStamp(0UL),
      _currentAnimIndex(0),
      _previousFrameIndex(-1),
      _previousAnimationIndex(-1)
{
}

AnimationComponent::~AnimationComponent()
{
}

void AnimationComponent::incParentTimeStamp(const U64 timestamp) {
    _parentTimeStamp += timestamp;
}

void AnimationComponent::update(const U64 deltaTime) {
    SGNComponent::update(deltaTime);

    if (_parentTimeStamp == _currentTimeStamp) {
        return;
    }

    _currentTimeStamp = _parentTimeStamp;

    if (!_playAnimations) {
        return;
    }

    if (_parentSGN.getNode<Object3D>()->updateAnimations(_parentSGN)) {
        _previousFrameIndex = _animator.frameIndexForTimeStamp(_currentAnimIndex, 
                                                               Time::MicrosecondsToSeconds<D32>(_currentTimeStamp));

        if ((_currentAnimIndex != _previousAnimationIndex) &&  _currentAnimIndex >= 0) {
            /*std::shared_ptr<AnimEvaluator> animation = getCurrentAnimation();
            if (animation->getBoneBuffer().expired()) {
                animation->initBuffers();
            }*/
            _previousAnimationIndex = _currentAnimIndex;
        }
    }
}

void AnimationComponent::resetTimers() {
    _currentTimeStamp = _parentTimeStamp = 0UL;
    _previousFrameIndex = -1;
    SGNComponent::resetTimers();
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const stringImpl& name) {
    U32 animIndex = 0;
    I32 oldIndex = _currentAnimIndex;

    if (_animator.animationID(name, animIndex)) {
        _currentAnimIndex = animIndex;
    }
    if (_currentAnimIndex == -1) {
        _currentAnimIndex = 0;
    }

    resetTimers();

    return oldIndex != _currentAnimIndex;
}

/// Select an animation by index
bool AnimationComponent::playAnimation(I32 pAnimIndex) {
    if (pAnimIndex >= to_int(_animator.animations().size())) {
        return false;  // no change, or the animations data is out of bounds
    }
    I32 oldIndex = _currentAnimIndex;
    _currentAnimIndex = pAnimIndex;  // only set this after the checks for good
                                     // data and the object was actually
                                     // inserted
    resetTimers();

    return oldIndex != _currentAnimIndex;
}

/// Select next available animation
bool AnimationComponent::playNextAnimation() {
    I32 oldIndex = _currentAnimIndex;
    _currentAnimIndex = ++_currentAnimIndex % _animator.animations().size();

    resetTimers();

    return oldIndex != _currentAnimIndex;
}

bool AnimationComponent::playPreviousAnimation() {
    I32 oldIndex = _currentAnimIndex;
    if (_currentAnimIndex == 0) {
        _currentAnimIndex = to_int(_animator.animations().size());
    }
    _currentAnimIndex = --_currentAnimIndex;

    resetTimers();

    return oldIndex != _currentAnimIndex;
}

const vectorImpl<Line>& AnimationComponent::skeletonLines() const {
    D32 animTimeStamp = Time::MicrosecondsToSeconds<D32>(_currentTimeStamp);
    // update possible animation
    return  _animator.skeletonLines(_currentAnimIndex, animTimeStamp);
}

bool AnimationComponent::onDraw(RenderStage currentStage) {
    if (!_playAnimations) {
        return true;
    }

    if (_previousAnimationIndex != -1) {
        std::shared_ptr<AnimEvaluator> animation = getAnimationByIndex(_previousAnimationIndex);
        std::shared_ptr<ShaderBuffer> boneBuffer = animation->getBoneBuffer().lock();

        assert(boneBuffer != nullptr);

        _parentSGN.getComponent<RenderingComponent>()->registerShaderBuffer(
            ShaderBufferLocation::BONE_TRANSFORMS,
            vec2<U32>(_previousFrameIndex, 1),
            *boneBuffer.get());
    }

    return true;
}

I32 AnimationComponent::frameCount(U32 animationID) const {
    return _animator.frameCount(animationID);
}

U32 AnimationComponent::boneCount() const {
    return to_uint(_animator.boneCount());
}

const vectorImpl<mat4<F32>>& AnimationComponent::transformsByIndex(
    U32 animationID, U32 index) const {
    return _animator.transforms(animationID, index);
}

const mat4<F32>& AnimationComponent::getBoneTransform(U32 animationID,
                                                      const D32 timeStamp,
                                                      const stringImpl& name) {
    Object3D* node = _parentSGN.getNode<Object3D>();
    assert(node != nullptr);

    if (node->getObjectType() != Object3D::ObjectType::SUBMESH ||
        (node->getObjectType() == Object3D::ObjectType::SUBMESH &&
         !node->hasFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))) {
        return _parentSGN.getComponent<PhysicsComponent>()->getWorldMatrix();
    }

    I32 frameIndex = _animator.frameIndexForTimeStamp(animationID, timeStamp);

    if (frameIndex == -1) {
        static mat4<F32> cacheIdentity;
        return cacheIdentity;
    }

    return _animator.transforms(animationID, frameIndex).at(_animator.boneIndex(name));
}

Bone* AnimationComponent::getBoneByName(const stringImpl& bname) const {
    return _animator.boneByName(bname);
}

std::shared_ptr<AnimEvaluator> 
AnimationComponent::getAnimationByIndex(I32 animationID) const {
    return _animator.animationByIndex(animationID);
}
};
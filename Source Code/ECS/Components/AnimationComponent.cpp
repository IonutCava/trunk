#include "stdafx.h"

#include "Headers/AnimationComponent.h"

#include "Managers/Headers/SceneManager.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

AnimationComponent::AnimationComponent(SceneGraphNode& parentSGN)
    : SGNComponent(parentSGN, "ANIMATION"),
      _playAnimations(true),
      _currentTimeStamp(0UL),
      _parentTimeStamp(0UL),
      _currentAnimIndex(0),
      _previousFrameIndex(0),
      _previousAnimationIndex(-1)
{
}

AnimationComponent::~AnimationComponent()
{
}

void AnimationComponent::incParentTimeStamp(const U64 timestamp) {
    _parentTimeStamp += timestamp;
}

void AnimationComponent::Update(const U64 deltaTimeUS) {
    if (!_animator || _parentTimeStamp == _currentTimeStamp) {
        return;
    }

    _currentTimeStamp = _parentTimeStamp;

    // Update Animations
    if (_playAnimations) {
        _parentSGN.getNode<Object3D>()->updateAnimations(_parentSGN);
        _previousFrameIndex = _animator->frameIndexForTimeStamp(_currentAnimIndex,
            Time::MicrosecondsToSeconds<D64>(_currentTimeStamp));

        if ((_currentAnimIndex != _previousAnimationIndex) && _currentAnimIndex >= 0) {
            _previousAnimationIndex = _currentAnimIndex;
        }
    }

    // Resolve IK
    //if (_resolveIK) {
        /// Use CCD to move target joints to target positions
    //}

    // Resolve ragdoll
    // if (_resolveRagdoll) {
        /// Use PhysX actor from RigidBodyComponent to feed new bone positions/orientation
        /// And read back ragdoll results to update transforms accordingly
    //}

    SGNComponent<AnimationComponent>::Update(deltaTimeUS);
}

void AnimationComponent::resetTimers() {
    _currentTimeStamp = _parentTimeStamp = 0UL;
    _previousFrameIndex = 0;
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const stringImpl& name) {
    if (!_animator) {
        return false;
    }

    U32 animIndex = 0;
    I32 oldIndex = _currentAnimIndex;

    if (_animator->animationID(name, animIndex)) {
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
    if (!_animator) {
        return false;
    }

    if (pAnimIndex >= to_I32(_animator->animations().size())) {
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
    if (!_animator) {
        return false;
    }

    I32 oldIndex = _currentAnimIndex;
    _currentAnimIndex = (_currentAnimIndex + 1 ) % _animator->animations().size();

    resetTimers();

    return oldIndex != _currentAnimIndex;
}

bool AnimationComponent::playPreviousAnimation() {
    if (!_animator) {
        return false;
    }

    I32 oldIndex = _currentAnimIndex;
    if (_currentAnimIndex == 0) {
        _currentAnimIndex = to_I32(_animator->animations().size());
    }
    --_currentAnimIndex;

    resetTimers();

    return oldIndex != _currentAnimIndex;
}

const vectorImpl<Line>& AnimationComponent::skeletonLines() const {
    assert(_animator != nullptr);

    D64 animTimeStamp = Time::MicrosecondsToSeconds<D64>(_currentTimeStamp);
    // update possible animation
    return  _animator->skeletonLines(_currentAnimIndex, animTimeStamp);
}

std::pair<vec2<U32>, ShaderBuffer*> AnimationComponent::getAnimationData() const {
    std::pair<vec2<U32>, ShaderBuffer*> ret(vec2<U32>(), nullptr);

    if (_playAnimations) {
        if (_previousAnimationIndex != -1) {
            ret.first.set(_previousFrameIndex, 1);
            ret.second = &getAnimationByIndex(_previousAnimationIndex).getBoneBuffer();
        }
    }

    return ret;
}
I32 AnimationComponent::frameCount(U32 animationID) const {
    return _animator != nullptr ? _animator->frameCount(animationID) : -1;
}

U32 AnimationComponent::boneCount() const {
    return _animator != nullptr ? to_U32(_animator->boneCount()) : 0;
}

const vectorImplBest<mat4<F32>>& AnimationComponent::transformsByIndex(U32 animationID, U32 index) const {
    assert(_animator != nullptr);

    return _animator->transforms(animationID, index);
}

const mat4<F32>& AnimationComponent::getBoneTransform(U32 animationID,
                                                      const D64 timeStamp,
                                                      const stringImpl& name) {
    const Object3D_ptr& node = _parentSGN.getNode<Object3D>();
    assert(node != nullptr);
    assert(_animator != nullptr);

    if (node->getObjectType() != Object3D::ObjectType::SUBMESH ||
        (node->getObjectType() == Object3D::ObjectType::SUBMESH &&
         !node->getObjectFlag(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED))) {
        return _parentSGN.get<TransformComponent>()->getWorldMatrix();
    }

    I32 frameIndex = _animator->frameIndexForTimeStamp(animationID, timeStamp);

    if (frameIndex == -1) {
        static mat4<F32> cacheIdentity;
        return cacheIdentity;
    }

    return _animator->transforms(animationID, frameIndex).at(_animator->boneIndex(name));
}

Bone* AnimationComponent::getBoneByName(const stringImpl& bname) const {
    return _animator != nullptr ? _animator->boneByName(bname) : nullptr;
}

AnimEvaluator& AnimationComponent::getAnimationByIndex(I32 animationID) const {
    assert(_animator != nullptr);

    return _animator->animationByIndex(animationID);
}
};
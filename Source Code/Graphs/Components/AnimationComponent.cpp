#include "Headers/AnimationComponent.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/AnimationController.h"

namespace Divide {

AnimationComponent::AnimationComponent(SceneAnimator* animator,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::SGN_COMP_ANIMATION, parentSGN),
      _animator(animator),
      _skeletonAvailable(false),
      _playAnimations(true),
      _currentTimeStamp(0.0),
      _currentAnimIndex(0) {
    assert(_animator != nullptr);

    _readBuffer = 1;
    _writeBuffer = 0;
    DIVIDE_ASSERT(_animator->GetBoneCount() <= Config::MAX_BONE_COUNT_PER_NODE,
                  "AnimationComponent error: Too many bones for current node! "
                  "Increase MAX_BONE_COUNT_PER_NODE in Config!");

    _boneTransformBuffer[_writeBuffer] = GFX_DEVICE.newSB("dvd_BoneTransforms");
    _boneTransformBuffer[_writeBuffer]->Create((U32)_animator->GetBoneCount(),
                                               sizeof(mat4<F32>));
    _boneTransformBuffer[_readBuffer] = GFX_DEVICE.newSB("dvd_BoneTransforms");
    _boneTransformBuffer[_readBuffer]->Create((U32)_animator->GetBoneCount(),
                                              sizeof(mat4<F32>));
}

AnimationComponent::~AnimationComponent() {
    MemoryManager::DELETE(_boneTransformBuffer[0]);
    MemoryManager::DELETE(_boneTransformBuffer[1]);
}

void AnimationComponent::update(const U64 deltaTime) {
    SGNComponent::update(deltaTime);

    D32 timeStamp =
        _playAnimations ? Time::MicrosecondsToSeconds<D32>(_elapsedTime) : 0.0;

    if (DOUBLE_COMPARE(timeStamp, _currentTimeStamp)) {
        return;
    }

    _readBuffer = (_readBuffer + 1) % 2;
    _writeBuffer = (_writeBuffer + 1) % 2;

    _currentTimeStamp = timeStamp;
    _lastFrameIndexes[_currentAnimIndex] =
        _animator->GetAnimationByIndex(_currentAnimIndex)
            .GetFrameIndexAt(_currentTimeStamp);

    Object3D* node = _parentSGN.getNode<Object3D>();
    node->updateAnimations(_parentSGN);
}

void AnimationComponent::resetTimers() {
    _currentTimeStamp = 0.0;
    SGNComponent::resetTimers();
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const stringImpl& name) {
    U32 animIndex = 0;
    I32 oldindex = _currentAnimIndex;

    if (_animator->GetAnimationID(name, animIndex)) {
        _currentAnimIndex = animIndex;
    }
    if (_currentAnimIndex == -1) {
        _currentAnimIndex = 0;
    }
    resetTimers();
    return oldindex != _currentAnimIndex;
}

/// Select an animation by index
bool AnimationComponent::playAnimation(I32 pAnimIndex) {
    if (pAnimIndex >= (I32)_animator->GetAnimations().size()) {
        return false;  // no change, or the animations data is out of bounds
    }
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = pAnimIndex;  // only set this after the checks for good
                                     // data and the object was actually
                                     // inserted
    resetTimers();
    return oldindex != _currentAnimIndex;
}

/// Select next available animation
bool AnimationComponent::playNextAnimation() {
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = ++_currentAnimIndex % _animator->GetAnimations().size();
    resetTimers();
    return oldindex != _currentAnimIndex;
}

void AnimationComponent::renderSkeleton() {
    if (!_skeletonAvailable ||
        (!GET_ACTIVE_SCENE()->renderState().drawSkeletons() &&
         !_parentSGN.getComponent<RenderingComponent>()->renderSkeleton())) {
        return;
    }

    // update possible animation
    const vectorImpl<Line>& skeletonLines =
        _animator->getSkeletonLines(_currentAnimIndex, _currentTimeStamp);
    // Submit skeleton to gpu
    GFX_DEVICE.drawLines(
        skeletonLines,
        _parentSGN.getComponent<PhysicsComponent>()->getWorldMatrix(),
        vec4<I32>(), false, true);
}

bool AnimationComponent::onDraw(RenderStage currentStage) {
    STUBBED(
        "BoneTransformBuffers: Replace Read/Write design with 3xSize single "
        "buffer with UpdateData + BindRange");

    _skeletonAvailable = false;

    vectorImpl<mat4<F32>>& animationTransforms =
        _animator->GetTransforms(_currentAnimIndex, _currentTimeStamp);
    _boneTransformBuffer[_writeBuffer]->UpdateData(
        0, animationTransforms.size() * sizeof(mat4<F32>),
        animationTransforms.data());
    _boneTransformBuffer[_readBuffer]->Bind(
        ShaderBufferLocation::SHADER_BUFFER_BONE_TRANSFORMS);

    if (!GFX_DEVICE.isCurrentRenderStage(RenderStage::DISPLAY_STAGE) ||
        !_playAnimations || _currentTimeStamp < 0.0) {
        return true;
    }
    // All animation data is valid, so we have a skeleton to render if needed
    _skeletonAvailable = true;

    return true;
}

I32 AnimationComponent::frameIndex(U32 animationID) const {
    frameIndexes::const_iterator it = _lastFrameIndexes.find(animationID);
    if (it != std::end(_lastFrameIndexes)) {
        return it->second;
    }

    return 0;
}

I32 AnimationComponent::frameCount(U32 animationID) const {
    return _animator->GetFrameCount(animationID);
}

U32 AnimationComponent::boneCount() const {
    return (U32)_animator->GetBoneCount();
}

const vectorImpl<mat4<F32>>& AnimationComponent::transformsByIndex(
    U32 animationID, U32 index) const {
    return _animator->GetTransformsByIndex(animationID, index);
}

const mat4<F32>& AnimationComponent::getBoneTransform(U32 animationID,
                                                      const stringImpl& name) {
    Object3D* node = _parentSGN.getNode<Object3D>();
    assert(node != nullptr);

    if (node->getObjectType() != Object3D::ObjectType::SUBMESH ||
        (node->getObjectType() == Object3D::ObjectType::SUBMESH &&
         !bitCompare(node->getFlagMask(), 
                     to_uint(Object3D::ObjectFlag::OBJECT_FLAG_SKINNED)))) {
        return _parentSGN.getComponent<PhysicsComponent>()->getWorldMatrix();
    }

    return currentBoneTransform(animationID, name);
}

const mat4<F32>& AnimationComponent::currentBoneTransform(
    U32 animationID, const stringImpl& name) {
    assert(_animator != nullptr);
    I32 boneIndex = _animator->GetBoneIndex(name);
    if (boneIndex == -1) {
        static mat4<F32> cacheIdentity;
        return cacheIdentity;
    }

    vectorImpl<mat4<F32>>& animationTransforms =
        _animator->GetTransformsByIndex(animationID,
                                        _lastFrameIndexes[animationID]);
    return animationTransforms[boneIndex];
}

Bone* AnimationComponent::getBoneByName(const stringImpl& bname) const {
    return _animator ? _animator->GetBoneByName(bname) : nullptr;
}

const AnimEvaluator& AnimationComponent::GetAnimationByIndex(
    I32 animationID) const {
    assert(_animator != nullptr);

    return _animator->GetAnimationByIndex(animationID);
}
};
#include "Headers/AnimationComponent.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/SceneAnimator.h"

namespace Divide {

AnimationComponent::AnimationComponent(SceneAnimator* animator,
                                       SceneGraphNode& parentSGN)
    : SGNComponent(SGNComponent::ComponentType::ANIMATION, parentSGN),
      _animator(animator),
      _skeletonAvailable(false),
      _playAnimations(true),
      _currentTimeStamp(0UL),
      _parentTimeStamp(0UL),
      _currentAnimIndex(0),
      _dataRange(0),
      _readOffset(-1),
      _writeOffset(2),
      _bufferSizeFactor(3)
{
    assert(_animator != nullptr);

    DIVIDE_ASSERT(_animator->boneCount() <= Config::MAX_BONE_COUNT_PER_NODE,
                  "AnimationComponent error: Too many bones for current node! "
                  "Increase MAX_BONE_COUNT_PER_NODE in Config!");

    _dataRange = static_cast<I32>(_animator->boneCount());

    I32 alignmentOffset = (_dataRange * sizeof(mat4<F32>)) % ShaderBuffer::getTargetDataAlignment();

    if (alignmentOffset > 0) {
        _dataRange += (ShaderBuffer::getTargetDataAlignment() - alignmentOffset) / sizeof(mat4<F32>);
    }

    _boneTransformBuffer = GFX_DEVICE.newSB("dvd_BoneTransforms"/*, true*/);
    _boneTransformBuffer->Create(_dataRange * _bufferSizeFactor, sizeof(mat4<F32>));
}

AnimationComponent::~AnimationComponent()
{
    MemoryManager::DELETE(_boneTransformBuffer);
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

    D32 animTimeStamp = Time::MicrosecondsToSeconds<D32>(_currentTimeStamp);

    if (_parentSGN.getNode<Object3D>()->updateAnimations(_parentSGN)) {
        I32 resultingFrameIndex = 0;
        const vectorImpl<mat4<F32>>& animationTransforms =
            _animator->transforms(_currentAnimIndex, animTimeStamp,
                                  resultingFrameIndex);

        I32& frameIndex = _lastFrameIndexes[_currentAnimIndex];
        bool updateBuffers = resultingFrameIndex != frameIndex;

        if (updateBuffers) {
            frameIndex = resultingFrameIndex;

            _boneTransformBuffer->UpdateData(_writeOffset * _dataRange,
                                             animationTransforms.size(),
                                             (const bufferPtr)animationTransforms.data());
            _writeOffset = (_writeOffset + 1) % _bufferSizeFactor;
            _readOffset = (_readOffset + 1) % _bufferSizeFactor;
        }
    }
}

void AnimationComponent::resetTimers() {
    _currentTimeStamp = _parentTimeStamp = 0UL;
    SGNComponent::resetTimers();
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const stringImpl& name) {
    U32 animIndex = 0;
    I32 oldindex = _currentAnimIndex;

    if (_animator->animationID(name, animIndex)) {
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
    if (pAnimIndex >= static_cast<I32>(_animator->animations().size())) {
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
    _currentAnimIndex = ++_currentAnimIndex % _animator->animations().size();
    resetTimers();
    return oldindex != _currentAnimIndex;
}

void AnimationComponent::renderSkeleton() {
    if (!_skeletonAvailable ||
        (!GET_ACTIVE_SCENE()->renderState().drawSkeletons() &&
         !_parentSGN.getComponent<RenderingComponent>()->renderSkeleton())) {
        return;
    }
    D32 animTimeStamp = Time::MicrosecondsToSeconds<D32>(_currentTimeStamp);
    // update possible animation
    const vectorImpl<Line>& skeletonLines =
        _animator->skeletonLines(_currentAnimIndex, animTimeStamp);
    // Submit skeleton to gpu
    IMPrimitive& prim = GFX_DEVICE.drawLines(
        skeletonLines, 2.0f,
        _parentSGN.getComponent<PhysicsComponent>()->getWorldMatrix(),
        vec4<I32>(), false, true);

    prim.name("Skeleton_" + _parentSGN.getName());
}

bool AnimationComponent::onDraw(RenderStage currentStage) {
    _skeletonAvailable = false;

    if (GFX_DEVICE.getRenderStage() != RenderStage::DISPLAY ||
        !_playAnimations) {
        return true;
    }
    // All animation data is valid, so we have a skeleton to render if needed
    _skeletonAvailable = true;

    _parentSGN.getComponent<RenderingComponent>()->registerShaderBuffer(
        ShaderBufferLocation::BONE_TRANSFORMS,
        vec2<ptrdiff_t>(_readOffset * _dataRange, _dataRange),
        *_boneTransformBuffer);

    return true;
}

I32 AnimationComponent::frameIndex(U32 animationID) const {
    FrameIndexes::const_iterator it = _lastFrameIndexes.find(animationID);
    if (it != std::end(_lastFrameIndexes)) {
        return it->second;
    }

    return 0;
}

I32 AnimationComponent::frameCount(U32 animationID) const {
    return _animator->frameCount(animationID);
}

U32 AnimationComponent::boneCount() const {
    return static_cast<U32>(_animator->boneCount());
}

const vectorImpl<mat4<F32>>& AnimationComponent::transformsByIndex(
    U32 animationID, U32 index) const {
    return _animator->transforms(animationID, index);
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
    U32 animationID, const stringImpl& name) const {

    assert(_animator != nullptr);
    I32 boneIndex = _animator->boneIndex(name);
    FrameIndexes::const_iterator it = _lastFrameIndexes.find(animationID);
    if (boneIndex == -1 || it == std::end(_lastFrameIndexes)) {
        static mat4<F32> cacheIdentity;
        return cacheIdentity;
    }

    return _animator->transforms(animationID, it->second).at(boneIndex);
}

Bone* AnimationComponent::getBoneByName(const stringImpl& bname) const {
    return _animator ? _animator->boneByName(bname) : nullptr;
}

const AnimEvaluator& AnimationComponent::GetAnimationByIndex(
    I32 animationID) const {
    assert(_animator != nullptr);

    return _animator->animationByIndex(animationID);
}

};
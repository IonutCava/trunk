#include "Headers/AnimationComponent.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/AnimationController.h"

AnimationComponent::AnimationComponent(SceneAnimator* animator, SceneGraphNode* const parentSGN) : SGNComponent(SGNComponent::SGN_COMP_ANIMATION, parentSGN),
                                                                  _animator(animator),
                                                                  _skeletonAvailable(false),
                                                                  _playAnimations(true),
                                                                  _currentTimeStamp(0.0),
                                                                  _currentAnimationID(0),
                                                                  _currentFrameIndex(0),
                                                                  _currentAnimIndex(0)
{
    assert(_animator != nullptr);

    _animationTransforms.clear();
    _animationTransforms.reserve(40);
}

AnimationComponent::~AnimationComponent()
{
}

void AnimationComponent::update(const U64 deltaTime) {
    SGNComponent::update(deltaTime);
    
    Object3D* node = _parentSGN->getNode<Object3D>();
    node->updateAnimations(_parentSGN);
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const std::string& name){
    U32 animIndex = 0;
    I32 oldindex = _currentAnimIndex;

    if (_animator->GetAnimationID(name, animIndex))
        _currentAnimIndex = animIndex;

    if (_currentAnimIndex == -1)
        _currentAnimIndex = 0;

    return oldindex != _currentAnimIndex;
}

/// Select an animation by index
bool AnimationComponent::playAnimation(I32  pAnimIndex){
    if (pAnimIndex >= (I32)_animator->GetAnimations().size()) return false;// no change, or the animations data is out of bounds
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = pAnimIndex;// only set this after the checks for good data and the object was actually inserted
    return oldindex != _currentAnimIndex;
}

/// Select next available animation
bool AnimationComponent::playNextAnimation() {
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = ++_currentAnimIndex % _animator->GetAnimations().size();
    return oldindex != _currentAnimIndex;
}

void AnimationComponent::renderSkeleton(){
    if (!_skeletonAvailable || !GET_ACTIVE_SCENE()->renderState().drawSkeletons()) return;
    // update possible animation
    _animator->setGlobalMatrix(_parentSGN->getTransform()->getGlobalMatrix());
    _animator->RenderSkeleton(_currentAnimIndex, _currentTimeStamp);
}

void AnimationComponent::onDraw(RenderStage currentStage) {
    _skeletonAvailable = false;

    if (!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE) || !_playAnimations)
        return;

    _currentTimeStamp = _playAnimations ? getUsToSec(_elapsedTime) : 0.0;

    //All animation data is valid, so we have a skeleton to render if needed
    _skeletonAvailable = true;

    _animationTransforms = _animator->GetTransforms(_currentAnimIndex, _currentTimeStamp);
}

U32 AnimationComponent::frameIndex() const {
    return _animator->GetFrameIndex(_currentAnimIndex);
}

U32 AnimationComponent::frameCount() const {
    return _animator->GetFrameCount(_currentAnimIndex);
}

const vectorImpl<mat4<F32> >& AnimationComponent::transformsByIndex(U32 index) const { 
    return _animator->GetTransformsByIndex(_currentAnimIndex, index);
}

const mat4<F32>& AnimationComponent::getBoneTransform(const std::string& name) {
    Object3D* node = _parentSGN->getNode<Object3D>();
    assert(node != nullptr);

    if (node->getType() != Object3D::SUBMESH || (node->getType() == Object3D::SUBMESH &&  node->getFlag() != Object3D::OBJECT_FLAG_SKINNED)){
        assert(_parentSGN->getTransform());
        return _parentSGN->getTransform()->getMatrix();
    }

    return currentBoneTransform(name);
}
const mat4<F32>& AnimationComponent::currentBoneTransform(const std::string& name){
    assert(_animator != nullptr);
    I32 boneIndex = _animator->GetBoneIndex(name);
    if (boneIndex == -1){
        static mat4<F32> cacheIdentity;
        return cacheIdentity;
    }

    return _animationTransforms[boneIndex];
}
Bone* AnimationComponent::getBoneByName(const std::string& bname) const {
    return _animator ? _animator->GetBoneByName(bname) : nullptr;
}
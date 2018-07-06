#include "Headers/AnimationComponent.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Geometry/Animations/Headers/AnimationController.h"

AnimationComponent::AnimationComponent(SceneAnimator* animator, SceneGraphNode* const parentSGN) : SGNComponent(SGNComponent::SGN_COMP_ANIMATION, parentSGN),
                                                                  _animator(animator),
                                                                  _skeletonAvailable(false),
                                                                  _playAnimations(true),
                                                                  _currentTimeStamp(-1.0),
                                                                  _currentAnimationID(0),
                                                                  _currentFrameIndex(0),
                                                                  _currentAnimIndex(0)
{
    assert(_animator != nullptr);

    _animationTransforms.clear();
    _animationTransforms.reserve(40);
    _readBuffer  = 1;
    _writeBuffer = 0;
    DIVIDE_ASSERT(_animator->GetBoneCount() <= Config::MAX_BONE_COUNT_PER_NODE, "AnimationComponent error: Too many bones for current node! Increase MAX_BONE_COUNT_PER_NODE in Config!"); 

    _boneTransformBuffer[_writeBuffer] = GFX_DEVICE.newSB();
    _boneTransformBuffer[_writeBuffer]->Create((U32)_animator->GetBoneCount(), sizeof(mat4<F32>));
    _boneTransformBuffer[_readBuffer] = GFX_DEVICE.newSB();
    _boneTransformBuffer[_readBuffer]->Create((U32)_animator->GetBoneCount(), sizeof(mat4<F32>));

}

AnimationComponent::~AnimationComponent()
{
    SAFE_DELETE(_boneTransformBuffer[0]);
    SAFE_DELETE(_boneTransformBuffer[1]);
}

void AnimationComponent::update(const U64 deltaTime) {
    SGNComponent::update(deltaTime);
    
    D32 timeStamp = _playAnimations ? getUsToSec(_elapsedTime) : 0.0;

    if(DOUBLE_COMPARE(timeStamp, _currentTimeStamp)) return;

    _readBuffer  = (_readBuffer + 1) % 2;
    _writeBuffer = (_writeBuffer + 1) % 2;

    _currentTimeStamp = timeStamp;

    _animationTransforms = _animator->GetTransforms(_currentAnimIndex, _currentTimeStamp);

    Object3D* node = _parentSGN->getNode<Object3D>();
    node->updateAnimations(_parentSGN);
}

void AnimationComponent::reset(){
    _currentTimeStamp = -1.0;
    SGNComponent::reset();
}

/// Select an animation by name
bool AnimationComponent::playAnimation(const std::string& name){
    U32 animIndex = 0;
    I32 oldindex = _currentAnimIndex;

    if (_animator->GetAnimationID(name, animIndex))
        _currentAnimIndex = animIndex;

    if (_currentAnimIndex == -1)
        _currentAnimIndex = 0;

    reset();
    return oldindex != _currentAnimIndex;
}

/// Select an animation by index
bool AnimationComponent::playAnimation(I32  pAnimIndex){
    if (pAnimIndex >= (I32)_animator->GetAnimations().size()) return false;// no change, or the animations data is out of bounds
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = pAnimIndex;// only set this after the checks for good data and the object was actually inserted
    reset();
    return oldindex != _currentAnimIndex;
}

/// Select next available animation
bool AnimationComponent::playNextAnimation() {
    I32 oldindex = _currentAnimIndex;
    _currentAnimIndex = ++_currentAnimIndex % _animator->GetAnimations().size();
    reset();
    return oldindex != _currentAnimIndex;
}

void AnimationComponent::renderSkeleton(){
    if (!_skeletonAvailable || (!GET_ACTIVE_SCENE()->renderState().drawSkeletons() && !_parentSGN->renderSkeleton())) {
        return;
    }

    // update possible animation
    const vectorImpl<Line >& skeletonLines = _animator->getSkeletonLines(_currentAnimIndex, _currentTimeStamp);
    // Submit skeleton to gpu
    GFX_DEVICE.drawLines(skeletonLines, _parentSGN->getWorldMatrix(), vec4<I32>(), false, true);
}

void AnimationComponent::onDraw(RenderStage currentStage) {
    STUBBED("BoneTransformBuffers: Replace Read/Write design with 3xSize single buffer with UpdateData + BindRange");

    _skeletonAvailable = false;

    _boneTransformBuffer[_writeBuffer]->UpdateData(0, _animationTransforms.size() * sizeof(mat4<F32>),
                                                   _animationTransforms.data(), true);
    _boneTransformBuffer[_readBuffer]->Bind(Divide::SHADER_BUFFER_BONE_TRANSFORMS);

    if (!GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE) || !_playAnimations || _currentTimeStamp < 0.0)
        return;

    //All animation data is valid, so we have a skeleton to render if needed
    _skeletonAvailable = true;
}

I32 AnimationComponent::frameIndex() const {
    return _animator->GetFrameIndex(_currentAnimIndex);
}

I32 AnimationComponent::frameCount() const {
    return _animator->GetFrameCount(_currentAnimIndex);
}

U32 AnimationComponent::boneCount()  const {
     return (U32)_animator->GetBoneCount(); 
}

const vectorImpl<mat4<F32> >& AnimationComponent::transformsByIndex(U32 index) const { 
    return _animator->GetTransformsByIndex(_currentAnimIndex, index);
}

const mat4<F32>& AnimationComponent::getBoneTransform(const std::string& name) {
    Object3D* node = _parentSGN->getNode<Object3D>();
    assert(node != nullptr);

    if (node->getObjectType() != Object3D::SUBMESH || (node->getObjectType() == Object3D::SUBMESH && !bitCompare(node->getFlagMask(), Object3D::OBJECT_FLAG_SKINNED))){
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
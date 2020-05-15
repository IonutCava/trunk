/*
Copyright (c) 2018 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once
#ifndef _ANIMATION_COMPONENT_H_
#define _ANIMATION_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/Line.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"
#include "Geometry/Animations/Headers/AnimationEvaluator.h"

namespace Divide {

class ShaderBuffer;
class AnimEvaluator;
class SceneGraphNode;

FWD_DECLARE_MANAGED_CLASS(SceneAnimator);
class AnimationComponent final : public BaseComponentType<AnimationComponent, ComponentType::ANIMATION> {
   public:
    explicit AnimationComponent(SceneGraphNode& parentSGN, PlatformContext& context);
    ~AnimationComponent() = default;

    /// Select an animation by name
    bool playAnimation(const stringImpl& name);
    /// Select an animation by index
    bool playAnimation(I32 pAnimIndex);
    /// Select next available animation
    bool playNextAnimation();
    /// Select previous available animation
    bool playPreviousAnimation();

    I32 frameCount(U32 animationID) const;

    U32 boneCount() const;
    Bone* getBoneByName(const stringImpl& bname) const;
    mat4<F32> getBoneTransform(U32 animationID, const D64 timeStamp, const stringImpl& name);
    const vectorEASTL<Line>& skeletonLines() const;
    std::pair<vec2<U32>, ShaderBuffer*> getAnimationData() const;
    AnimEvaluator& getAnimationByIndex(I32 animationID) const;
    const BoneTransform& transformsByIndex(U32 animationID, U32 index) const;

    void resetTimers();
    void incParentTimeStamp(const U64 timestamp);
    void setParentTimeStamp(const U64 timestamp);

    inline U64 animationTimeStamp() const noexcept { return _currentTimeStamp; }
    inline U32 frameIndex() const noexcept { return _previousFrameIndex; }
    inline I32 frameCount() const noexcept { return frameCount(_currentAnimIndex); }

    inline const BoneTransform& transformsByIndex(U32 index) const { return transformsByIndex(_currentAnimIndex, index); }
    inline AnimEvaluator& getCurrentAnimation() const { return getAnimationByIndex(animationIndex()); }
    inline void updateAnimator(const SceneAnimator_ptr& animator) noexcept { _animator = animator; }
    inline I32 animationIndex() const noexcept { return _currentAnimIndex; }

    PROPERTY_R(bool, showSkeleton, false);
    PROPERTY_RW(bool, playAnimations, true);

   protected:
    friend class AnimationSystem;
    template<typename T, typename U>
    friend class ECSSystem;
    void Update(const U64 deltaTimeUS) final;

   protected:
    /// Pointer to the mesh's animator. Owned by the mesh!
    std::shared_ptr<SceneAnimator> _animator = nullptr;
    /// Current animation index for the current SGN
    I32 _currentAnimIndex = -1;
    /// Current animation timestamp for the current SGN
    U64 _currentTimeStamp = 0UL;
    /// Previous frame index. Gets reset to -1 when animation changes
    U32 _previousFrameIndex = 0;
    /// Previous animation index
    I32 _previousAnimationIndex = -1;
    /// Parent time stamp (e.g. Mesh). 
    /// Should be identical for all nodes of the same level with the same parent
    U64 _parentTimeStamp = 0ul;
};

INIT_COMPONENT(AnimationComponent);

};  // namespace Divide
#endif
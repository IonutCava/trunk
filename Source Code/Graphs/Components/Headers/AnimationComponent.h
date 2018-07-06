/*
Copyright (c) 2017 DIVIDE-Studio
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

#ifndef _ANIMATION_COMPONENT_H_
#define _ANIMATION_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/Line.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Bone;
class ShaderBuffer;
class AnimEvaluator;
class SceneAnimator;
class SceneGraphNode;
class AnimationComponent : public SGNComponent {
   public:
    inline void updateAnimator(const std::shared_ptr<SceneAnimator>& animator) {
        _animator = animator;
    }

    bool onRender(const SceneRenderState& sceneRenderState,
                  const RenderStagePass& renderStagePass) override;

    void update(const U64 deltaTimeUS) override;

    /// Select an animation by name
    bool playAnimation(const stringImpl& name);
    /// Select an animation by index
    bool playAnimation(I32 pAnimIndex);
    /// Select next available animation
    bool playNextAnimation();
    /// Select previous available animation
    bool playPreviousAnimation();

    inline U64 animationTimeStamp() const { return _currentTimeStamp;  }
    inline U32 frameIndex() const { return _previousFrameIndex; }
    inline I32 frameCount() const { return frameCount(_currentAnimIndex); }
    I32 frameCount(U32 animationID) const;

    inline const vectorImplBest<mat4<F32> >& transformsByIndex(U32 index) const {
        return transformsByIndex(_currentAnimIndex, index);
    }

    const vectorImplBest<mat4<F32> >& transformsByIndex(U32 animationID, U32 index) const;

    U32 boneCount() const;
    Bone* getBoneByName(const stringImpl& bname) const;

    const mat4<F32>& getBoneTransform(U32 animationID,
                                      const D64 timeStamp,
                                      const stringImpl& name);

    inline bool playAnimations() const { return _playAnimations; }
    inline void playAnimations(bool state) { _playAnimations = state; }

    inline I32 animationIndex() const { return _currentAnimIndex; }

    AnimEvaluator& getAnimationByIndex(I32 animationID) const;

    inline AnimEvaluator& getCurrentAnimation() const {
        return getAnimationByIndex(animationIndex());
    }

    void resetTimers();
    void incParentTimeStamp(const U64 timestamp);

    const vectorImpl<Line>& skeletonLines() const;
   protected:
    friend class SceneGraphNode;
    AnimationComponent(SceneGraphNode& parentSGN);
    ~AnimationComponent();

   protected:
    /// Pointer to the mesh's animator. Owned by the mesh!
    std::shared_ptr<SceneAnimator> _animator;
    /// Current animation index for the current SGN
    I32 _currentAnimIndex;
    /// Current animation timestamp for the current SGN
    U64 _currentTimeStamp;
    /// Previous frame index. Gets reset to -1 when animation changes
    U32 _previousFrameIndex;
    /// Previous animation index
    I32 _previousAnimationIndex;
    /// Parent time stamp (e.g. Mesh). 
    /// Should be identical for all nodes of the same level with the same parent
    U64 _parentTimeStamp;
    /// Animation playback toggle
    bool _playAnimations;
};

};  // namespace Divide
#endif
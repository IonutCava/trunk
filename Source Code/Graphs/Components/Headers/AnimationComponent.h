/*
Copyright (c) 2014 DIVIDE-Studio
Copyright (c) 2009 Ionut Cava

This file is part of DIVIDE Framework.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef _ANIMATION_COMPONENT_H_
#define _ANIMATION_COMPONENT_H_

#include "SGNComponent.h"
#include "Core/Math/Headers/MathMatrices.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

class Bone;
class ShaderBuffer;
class SceneAnimator;
class SceneGraphNode;
class AnimationComponent : public SGNComponent {

public:
    typedef hashMapImpl<U32 /*frame index*/, BoundingBox>  boundingBoxPerFrame;
    typedef hashMapImpl<U32 /*animation ID*/, boundingBoxPerFrame> boundingBoxPerAnimation;

    AnimationComponent(SceneAnimator* animator, SceneGraphNode* const parentSGN);
    ~AnimationComponent();
    bool onDraw(RenderStage currentStage);
    
    void update(const U64 deltaTime);

    void renderSkeleton();
    /// Select an animation by name
    bool playAnimation(const stringImpl& name);
    /// Select an animation by index
    bool playAnimation(I32  pAnimIndex);
    /// Select next available animation
    bool playNextAnimation();
    
   
    I32 frameIndex() const;
    I32 frameCount() const;
    U32 boneCount()  const;

    const vectorImpl<mat4<F32> >& transformsByIndex(U32 index) const;
    const mat4<F32>& currentBoneTransform(const stringImpl& name);
    Bone* getBoneByName(const stringImpl& bname) const;
    const mat4<F32>& getBoneTransform(const stringImpl& name);

    inline bool playAnimations()           const { return _playAnimations; }
    inline void playAnimations(bool state)       { _playAnimations = state; }

    
    inline I32  animationIndex()      const { return _currentAnimIndex; }
    inline const vectorImpl<mat4<F32> >& animationTransforms() const { return _animationTransforms; }

    inline boundingBoxPerFrame& getBBoxesForAnimation(U32 animationId) { return _boundingBoxes[animationId]; }

    void resetTimers();

protected:
    /// Pointer to the mesh's animator. Owned by the mesh!
    SceneAnimator* _animator;
    /// Current animation index for the current SGN
    I32 _currentAnimIndex;
    /// Current animation timestamp for the current SGN
    D32 _currentTimeStamp;
    ///Animations (current and previous for interpolation)
    vectorImpl<mat4<F32> > _animationTransforms;
    vectorImpl<mat4<F32> > _animationTransformsPrev;
    /// Current animation ID
    U32 _currentAnimationID;
    /// Current animation frame wrapped in animation time [0 ... 1]
    U32 _currentFrameIndex;
    /// Does the mesh have a valid skeleton?
    bool _skeletonAvailable; 
    /// Animation playback toggle
    bool _playAnimations;
    /// Animation timestamp changed
    bool _updateAnimations;
    ///BoundingBoxes for every frame
    boundingBoxPerFrame _bbsPerFrame;
    ///store a map of bounding boxes for every animation at every frame
    boundingBoxPerAnimation _boundingBoxes;
    ///used to upload bone data to the gpu
    ShaderBuffer* _boneTransformBuffer[2];
    ///used to switch bone buffers around per frame
    U32 _readBuffer;
    U32 _writeBuffer;
};

}; //namespace Divide
#endif
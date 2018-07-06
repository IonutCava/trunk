/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SKINNED_SUBMESH_H_
#define _SKINNED_SUBMESH_H_

#include "SubMesh.h"
#include "Platform/Threading/Headers/Task.h"
namespace Divide {

class AnimationComponent;
class SkinnedSubMesh : public SubMesh {
    typedef vectorImpl<BoundingBox> BoundingBoxPerFrame;
    typedef vectorImpl<BoundingBoxPerFrame> BoundingBoxPerAnimation;
    typedef vectorImpl<AtomicWrapper<bool>>  BoundingBoxPerAnimationStatus;

   public:
    SkinnedSubMesh(const stringImpl& name);
    ~SkinnedSubMesh();

   public:
    void postLoad(SceneGraphNode& sgn);

   protected:
    void updateAnimations(SceneGraphNode& sgn);

   private:
    void sceneUpdate(const U64 deltaTime,
                     SceneGraphNode& sgn,
                     SceneState& sceneState) override;
    void computeBoundingBoxForCurrentFrame(SceneGraphNode& sgn);
    void buildBoundingBoxesForAnimCompleted(U32 animationIndex);
    void buildBoundingBoxesForAnim(const std::atomic_bool& stopRequested,
                                   U32 animationIndex,
                                   AnimationComponent* const animComp);

   private:
    std::shared_ptr<SceneAnimator> _parentAnimatorPtr;
    /// Build status of bounding boxes for each animation (true if BBs are available)
    BoundingBoxPerAnimationStatus _boundingBoxesAvailable;
    /// Build status of bounding boxes for each animation (true if BBs are being computed)
    BoundingBoxPerAnimationStatus _boundingBoxesComputing;
    /// BoundingBoxes for every frame
    BoundingBoxPerFrame _bbsPerFrame;
    /// store a map of bounding boxes for every animation at every frame
    BoundingBoxPerAnimation _boundingBoxes;
};

TYPEDEF_SMART_POINTERS_FOR_CLASS(SkinnedSubMesh);

};  // namespace Divide

#endif
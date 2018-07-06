/*
   Copyright (c) 2015 DIVIDE-Studio
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

namespace Divide {

class AnimationComponent;
class SkinnedSubMesh : public SubMesh {
    typedef hashMapImpl<U32 /*frame index*/, BoundingBox> boundingBoxPerFrame;
    typedef hashMapImpl<U32 /*animation ID*/, boundingBoxPerFrame>
        boundingBoxPerAnimation;
    typedef hashMapImpl<U32 /*animation ID*/, std::atomic_bool /*computed BBs*/>
        boundingBoxPerAnimationStatus;

   public:
    SkinnedSubMesh(const stringImpl& name);
    ~SkinnedSubMesh();

   public:
    void postLoad(SceneGraphNode& sgn);

   protected:
    bool updateAnimations(SceneGraphNode& sgn);

   private:
    bool getBoundingBoxForCurrentFrame(SceneGraphNode& sgn);
    void buildBoundingBoxesForAnimCompleted(U32 animationIndex);
    void buildBoundingBoxesForAnim(U32 animationIndex,
                                   AnimationComponent* const animComp);

   private:
    SceneAnimator* _parentAnimatorPtr;
    vectorImpl<vec3<F32> > _origVerts;
    vectorImpl<vec3<F32> > _origNorms;
    /// This becomes true only while computing bbs for any animation
    std::atomic_bool _buildingBoundingBoxes;
    /// Build status of bounding boxes for each animation (true if BBs are available)
    boundingBoxPerAnimationStatus _boundingBoxesAvailable;
    /// Build status of bounding boxes for each animation (true if BBs are being computed)
    boundingBoxPerAnimationStatus _boundingBoxesComputing;
    /// BoundingBoxes for every frame
    boundingBoxPerFrame _bbsPerFrame;
    /// store a map of bounding boxes for every animation at every frame
    boundingBoxPerAnimation _boundingBoxes;
};

};  // namespace Divide

#endif
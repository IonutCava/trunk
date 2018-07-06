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

#ifndef _SKINNED_SUBMESH_H_
#define _SKINNED_SUBMESH_H_

#include "SubMesh.h"
class AnimationComponent;
class SkinnedSubMesh : public SubMesh {
public:
    SkinnedSubMesh(const std::string& name);
    ~SkinnedSubMesh();

public:
    void postLoad(SceneGraphNode* const sgn);

    inline SceneAnimator* getAnimator() { return _animator; }

protected:
    /// Called from SceneGraph "sceneUpdate"
    void sceneUpdate(const U64 deltaTime, SceneGraphNode* const sgn, SceneState& sceneState);
    void updateAnimations(SceneGraphNode* const sgn);
    void drawReset(SceneGraphNode* const sgn);
    
private:
    void updateBBatCurrentFrame(SceneGraphNode* const sgn);
    
private:
    vectorImpl<vec3<F32> > _origVerts;
    vectorImpl<vec3<F32> > _origNorms;
    bool _softwareSkinning;
    /// Animation player to animate the mesh if necessary
    SceneAnimator* _animator;

    ///BoundingBoxes for every frame
    typedef Unordered_map<U32 /*frame index*/, BoundingBox>  boundingBoxPerFrame;
    boundingBoxPerFrame _bbsPerFrame;
    ///store a map of bounding boxes for every animation at every frame
    Unordered_map<U32 /*animation ID*/, boundingBoxPerFrame> _boundingBoxes;
};
#endif
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

/*Code references:
    http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#ifndef SCENE_ANIMATOR_H_
#define SCENE_ANIMATOR_H_

#include "AnimationEvaluator.h"

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Divide {

class SceneAnimator {
   public:
    typedef hashMapImpl<I32 /*frameIndex*/, vectorAlg::vecSize /*vectorIntex*/>
        LineMap;
    typedef hashMapImpl<I32 /*animationID*/, LineMap> LineCollection;

    SceneAnimator();
    ~SceneAnimator();

    /// This must be called to fill the SceneAnimator with valid data
    bool init(const aiScene* pScene);
    /// Frees all memory and initializes everything to a default state
    void release();
    void save(std::ofstream& file);
    void load(std::ifstream& file);
    /// Lets the caller know if there is a skeleton present
    inline bool hasSkeleton() const { return _skeleton != nullptr; }
    /// The next two functions are good if you want to change the direction of
    /// the current animation.
    /// You could use a forward walking animation and reverse it to get a
    /// walking backwards
    inline void playAnimationForward(I32 animationIndex) {
        _animations[animationIndex].playAnimationForward(true);
    }
    inline void playAnimationBackward(I32 animationIndex) {
        _animations[animationIndex].playAnimationForward(false);
    }
    /// This function will adjust the current animations speed by a percentage.
    /// So, passing 100, would do nothing, passing 50, would decrease the speed
    /// by half, and 150 increase it by 50%
    inline void adjustAnimationSpeedBy(I32 animationIndex, const D32 percent) {
        AnimEvaluator& animation = _animations.at(animationIndex);
        animation.ticksPerSecond(animation.ticksPerSecond() * (percent / 100.0));
    }
    /// This will set the animation speed
    inline void adjustAnimationSpeedTo(I32 animationIndex,
                                       const D32 tickspersecond) {
        _animations[animationIndex].ticksPerSecond(tickspersecond);
    }
    /// Get the animationspeed... in ticks per second
    inline D32 animationSpeed(I32 animationIndex) const {
        return _animations[animationIndex].ticksPerSecond();
    }

    /// Get the transforms needed to pass to the vertex shader.
    /// This will wrap the dt value passed, so it is safe to pass 50000000 as a
    /// valid number
    inline I32 frameIndexForTimeStamp(I32 animationIndex, const D32 dt) const {
        return _animations[animationIndex].frameIndexAt(dt);
    }

    inline const vectorImpl<mat4<F32>>& transforms(I32 animationIndex,
                                                   U32 index) const {
        return _animations[animationIndex].transforms(index);
    }

    inline const AnimEvaluator& animationByIndex(I32 animationIndex) const {
        return _animations[animationIndex];
    }

    inline U32 frameCount(I32 animationIndex) const {
        return _animations[animationIndex].frameCount();
    }

    inline const vectorImpl<AnimEvaluator>& animations() const {
        return _animations;
    }

    inline const stringImpl& animationName(I32 animationIndex) const {
        return _animations[animationIndex].name();
    }

    inline bool animationID(const stringImpl& animationName, U32& ID) {
        hashMapImpl<stringImpl, U32>::iterator itr =
            _animationNameToID.find(animationName);
        if (itr != std::end(_animationNameToID)) {
            ID = itr->second;
            return true;
        }
        return false;
    }
    /// GetBoneTransform will return the matrix of the bone given its name and
    /// the time.
    /// Be careful with this to make sure and send the correct dt. If the dt is
    /// different from what the model is currently at,
    /// the transform will be off
    inline const mat4<F32>& boneTransform(I32 animationIndex, const D32 dt,
                                          const stringImpl& bname) {
        I32 boneID = boneIndex(bname);
        if (boneID != -1) {
            return boneTransform(animationIndex, dt, boneID);
        }

        _boneTransformCache.identity();
        return _boneTransformCache;
    }

    /// Same as above, except takes the index
    inline const mat4<F32>& boneTransform(I32 animationIndex, const D32 dt,
                                          I32 bindex) {
        if (bindex != -1) {
            return _animations[animationIndex].transforms(dt).at(bindex);
        }

        _boneTransformCache.identity();
        return _boneTransformCache;
    }

    /// Get the bone's global transform
    inline const mat4<F32>& boneOffsetTransform(const stringImpl& bname) {
        Bone* bone = boneByName(bname);
        if (bone != nullptr) {
            AnimUtils::TransformMatrix(bone->_offsetMatrix, _boneTransformCache);
        }
        return _boneTransformCache;
    }

    Bone* boneByName(const stringImpl& name) const;
    /// GetBoneIndex will return the index of the bone given its name.
    /// The index can be used to index directly into the vector returned from
    /// GetTransform
    I32 boneIndex(const stringImpl& bname) const;
    const vectorImpl<Line>& skeletonLines(I32 animationIndex, const D32 dt);

    inline size_t boneCount() const {
        return _skeletonDepthCache;
    }

   private:
    /// I/O operations
    void saveSkeleton(std::ofstream& file, Bone* pNode);
    Bone* loadSkeleton(std::ifstream& file, Bone* pNode);

    void updateTransforms(Bone* pNode);
    void calculate(I32 animationIndex, const D32 pTime);
    void calcBoneMatrices();
    /// Calculates the global transformation matrix for the given internal node
    void calculateBoneToWorldTransform(Bone* pInternalNode);
    void extractAnimations(const aiScene* pScene);
    /// Recursively creates an internal node structure matching the current
    /// scene and animation.
    Bone* createBoneTree(aiNode* pNode, Bone* parent);

    I32 createSkeleton(Bone* piNode, const aiMatrix4x4& parent,
                       vectorImpl<Line>& lines);

   private:
    /// Root node of the internal scene structure
    Bone* _skeleton;
    I32   _skeletonDepthCache;
    /// A vector that holds each animation
    vectorImpl<AnimEvaluator> _animations;
    /// find animations quickly
    hashMapImpl<stringImpl, U32> _animationNameToID;
    /// temp array of transforms
    vectorImpl<aiMatrix4x4> _transforms;

    mat4<F32> _boneTransformCache;
    LineCollection _skeletonLines;
    vectorImpl<vectorImpl<Line>> _skeletonLinesContainer;
};

};  // namespace Divide

#endif // SCENE_ANIMATOR_H_

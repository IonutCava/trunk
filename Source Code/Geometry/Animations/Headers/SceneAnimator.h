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

/*Code references:
    http://nolimitsdesigns.com/game-design/open-asset-import-library-animation-loader/
*/

#ifndef SCENE_ANIMATOR_H_
#define SCENE_ANIMATOR_H_

#include "AnimationEvaluator.h"
#include "Core/Math/Headers/Line.h"

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Divide {

namespace Attorney {
    class SceneAnimatorMeshImporter;
};

/// Calculates the global transformation matrix for the given internal node
void calculateBoneToWorldTransform(Bone* pInternalNode);
class ByteBuffer;
class MeshImporter;
class PlatformContext;
class SceneAnimator {
    friend class Attorney::SceneAnimatorMeshImporter;
   public:
    // index = frameIndex; entry = vectorIndex;
    typedef vectorImpl<I32> LineMap;
    // index = animationID;
    typedef vectorImpl<LineMap> LineCollection;

    SceneAnimator();
    ~SceneAnimator();

    /// This must be called to fill the SceneAnimator with valid data
    bool init(PlatformContext& context, Bone* skeleton, const vectorImpl<Bone*>& bones);
    /// Frees all memory and initializes everything to a default state
    void release(bool releaseAnimations = true);
    void save(PlatformContext& context, ByteBuffer& dataOut) const;
    void load(PlatformContext& context, ByteBuffer& dataIn);
    /// Lets the caller know if there is a skeleton present
    inline bool hasSkeleton() const { return _skeleton != nullptr; }
    /// The next two functions are good if you want to change the direction of
    /// the current animation.
    /// You could use a forward walking animation and reverse it to get a
    /// walking backwards
    inline void playAnimationForward(I32 animationIndex) {
        _animations[animationIndex]->playAnimationForward(true);
    }
    inline void playAnimationBackward(I32 animationIndex) {
        _animations[animationIndex]->playAnimationForward(false);
    }
    /// This function will adjust the current animations speed by a percentage.
    /// So, passing 100, would do nothing, passing 50, would decrease the speed
    /// by half, and 150 increase it by 50%
    inline void adjustAnimationSpeedBy(I32 animationIndex, const D64 percent) {
        std::shared_ptr<AnimEvaluator>& animation = _animations.at(animationIndex);
        animation->ticksPerSecond(animation->ticksPerSecond() * (percent / 100.0));
    }
    /// This will set the animation speed
    inline void adjustAnimationSpeedTo(I32 animationIndex,
                                       const D64 tickspersecond) {
        _animations[animationIndex]->ticksPerSecond(tickspersecond);
    }
    /// Get the animationspeed... in ticks per second
    inline D64 animationSpeed(I32 animationIndex) const {
        return _animations[animationIndex]->ticksPerSecond();
    }

    /// Get the transforms needed to pass to the vertex shader.
    /// This will wrap the dt value passed, so it is safe to pass 50000000 as a
    /// valid number
    inline I32 frameIndexForTimeStamp(I32 animationIndex, const D64 dt) const {
        return _animations[animationIndex]->frameIndexAt(dt);
    }

    inline const vectorImplAligned<mat4<F32>>& transforms(I32 animationIndex, U32 index) const {
        return _animations[animationIndex]->transforms(index);
    }

    inline const AnimEvaluator& animationByIndex(I32 animationIndex) const {
        assert(IS_IN_RANGE_INCLUSIVE(animationIndex, 0, to_int(_animations.size()) - 1));
        const std::shared_ptr<AnimEvaluator>& animation = _animations.at(animationIndex);
        assert(animation != nullptr);
        return *animation;
    }

    inline AnimEvaluator& animationByIndex(I32 animationIndex) {
        assert(IS_IN_RANGE_INCLUSIVE(animationIndex, 0, to_int(_animations.size()) - 1));
        const std::shared_ptr<AnimEvaluator>& animation = _animations.at(animationIndex);
        assert(animation != nullptr);
        return *animation;
    }

    inline U32 frameCount(I32 animationIndex) const {
        return _animations[animationIndex]->frameCount();
    }

    inline const vectorImpl<std::shared_ptr<AnimEvaluator>>& animations() const {
        return _animations;
    }

    inline const stringImpl& animationName(I32 animationIndex) const {
        return _animations[animationIndex]->name();
    }

    inline bool animationID(const stringImpl& animationName, U32& ID) {
        hashMapImpl<U64, U32>::iterator itr =
            _animationNameToID.find(_ID_RT(animationName));
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
    inline const mat4<F32>& boneTransform(I32 animationIndex, const D64 dt,
                                          const stringImpl& bname) {
        I32 boneID = boneIndex(bname);
        if (boneID != -1) {
            return boneTransform(animationIndex, dt, boneID);
        }

        _boneTransformCache.identity();
        return _boneTransformCache;
    }

    /// Same as above, except takes the index
    inline const mat4<F32>& boneTransform(I32 animationIndex, const D64 dt,
                                          I32 bindex) {
        if (bindex != -1) {
            return _animations[animationIndex]->transforms(dt).at(bindex);
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
    const vectorImpl<Line>& skeletonLines(I32 animationIndex, const D64 dt);

    /// Returns the frame count of the longest registered animation
    inline U32 getMaxAnimationFrames() const {
        return _maximumAnimationFrames;
    }

    inline size_t boneCount() const {
        return _skeletonDepthCache;
    }

   private:
    bool init(PlatformContext& context);
    /// I/O operations
    void saveSkeleton(ByteBuffer& dataOut, Bone* pNode) const;
    Bone* loadSkeleton(ByteBuffer& dataIn, Bone* pNode);

    void updateTransforms(Bone* pNode);
    void calculate(I32 animationIndex, const D64 pTime);
    I32 createSkeleton(Bone* piNode,
                       const aiMatrix4x4& parent,
                       vectorImpl<Line>& lines,
                       bool rowMajor = false);

   private:
    /// Frame count of the longest registered animation
    U32 _maximumAnimationFrames;
    /// Root node of the internal scene structure
    Bone* _skeleton;
    I32   _skeletonDepthCache;
    vectorImpl<Bone*> _bones;
    /// A vector that holds each animation
    vectorImpl<std::shared_ptr<AnimEvaluator>> _animations;
    /// find animations quickly
    hashMapImpl<U64, U32> _animationNameToID;
    /// temp array of transforms
    vectorImpl<aiMatrix4x4> _transforms;
    mat4<F32> _boneTransformCache;
    LineCollection _skeletonLines;
    vectorImpl<vectorImpl<Line>> _skeletonLinesContainer;
};

namespace Attorney {
    class SceneAnimatorMeshImporter {
        private:
        static void registerAnimations(SceneAnimator& animator, const vectorImpl<std::shared_ptr<AnimEvaluator>>& animations) {
            size_t animationCount = animations.size();
            animator._animations.reserve(animationCount);
            for (size_t i = 0; i < animationCount; ++i) {
                animator._animations.push_back(animations[i]);
                hashAlg::emplace(animator._animationNameToID, _ID_RT(animator._animations[i]->name()), to_uint(i));
            }
        }

        friend class Divide::MeshImporter;
    };
};

};  // namespace Divide

#endif // SCENE_ANIMATOR_H_

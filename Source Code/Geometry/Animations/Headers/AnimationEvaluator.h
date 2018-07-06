/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef ANIMATION_EVALUATOR_H_
#define ANIMATION_EVALUATOR_H_

#include "Bone.h"
#include <assimp/anim.h>

struct aiAnimation;

namespace Divide {
class ByteBuffer;
class ShaderBuffer;

class AnimationChannel {
   public:
    AnimationChannel()
        : _nameKey(0ULL),
          _numPositionKeys(0U),
          _numRotationKeys(0U),
          _numScalingKeys(0U)
    {
    }

    U64 _nameKey;
    stringImpl _name;
    vector<aiVectorKey> _positionKeys;
    vector<aiQuatKey> _rotationKeys;
    vector<aiVectorKey> _scalingKeys;
    /** The number of position keys */
    U32 _numPositionKeys;
    U32 _numRotationKeys;
    U32 _numScalingKeys;
};

class GFXDevice;
class AnimEvaluator {
   public:
    AnimEvaluator();
    AnimEvaluator(const aiAnimation* pAnim, U32 idx);
    ~AnimEvaluator();

    void evaluate(const D64 dt, Bone* skeleton);

    I32 frameIndexAt(const D64 elapsedTime) const;

    
    inline U32 frameCount() const {
        return to_U32(_transforms.size());
    }

    inline vector<vectorBest<mat4<F32>>>& transforms() {
        return _transforms;
    }
    
    inline const vector<vectorBest<mat4<F32>>>& transforms() const {
        return _transforms;
    }

    inline vectorBest<mat4<F32>>& transforms(const U32 frameIndex) {
        assert(frameIndex < to_U32(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline const vectorBest<mat4<F32>>& transforms(const U32 frameIndex) const {
        assert(frameIndex < to_U32(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline vectorBest<mat4<F32>>& transforms(const D64 elapsedTime,
                                                    I32& resultingFrameIndex) {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_U32(resultingFrameIndex));
    }

    inline vectorBest<mat4<F32>>& transforms(const D64 elapsedTime) {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    inline const vectorBest<mat4<F32>>& transforms(const D64 elapsedTime,
                                                          I32& resultingFrameIndex) const {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_U32(resultingFrameIndex));
    }

    inline const vectorBest<mat4<F32>>& transforms(const D64 elapsedTime) const {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    inline void playAnimationForward(bool state) {
        _playAnimationForward = state;
    }

    inline bool playAnimationForward() const {
        return _playAnimationForward;
    }

    inline void ticksPerSecond(D64 tickCount) {
        _ticksPerSecond = tickCount;
    }

    inline D64 ticksPerSecond() const {
        return _ticksPerSecond;
    }

    inline const stringImpl& name() const {
        return _name;
    }

    inline D64 duration() const {
        return _duration;
    }

    bool initBuffers(GFXDevice& context);

    static void save(const AnimEvaluator& evaluator, ByteBuffer& dataOut);
    static void load(AnimEvaluator& evaluator, ByteBuffer& dataIn);

    ShaderBuffer& getBoneBuffer() const {
        return *_boneTransformBuffer;
    }

   protected:
    stringImpl _name;
    /// Array to return transformations results inside.
    vector<vectorBest<mat4<F32>>> _transforms;
    /// play forward == true, play backward == false
    bool _playAnimationForward;
    D64 _lastTime;
    D64 _duration;
    D64 _ticksPerSecond;

   private:
    vector<vec3<U32>> _lastPositions;
    /// vector that holds all bone channels
    vector<AnimationChannel> _channels;
    /// GPU buffer to hold bone transforms
    ShaderBuffer* _boneTransformBuffer;
};

};  // namespace Divide

#endif
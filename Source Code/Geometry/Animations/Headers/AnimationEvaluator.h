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

#ifndef ANIMATION_EVALUATOR_H_
#define ANIMATION_EVALUATOR_H_

#include "Bone.h"
#include <assimp/anim.h>
#include "Utility/Headers/HashMap.h"

struct aiAnimation;

namespace Divide {

class AnimationChannel {
   public:
    stringImpl _name;
    vectorImpl<aiVectorKey> _positionKeys;
    vectorImpl<aiQuatKey> _rotationKeys;
    vectorImpl<aiVectorKey> _scalingKeys;
    /** The number of position keys */
    U32 _numPositionKeys;
    U32 _numRotationKeys;
    U32 _numScalingKeys;
};

class AnimEvaluator {
   public:
    AnimEvaluator()
        : _lastTime(0.0),
          _ticksPerSecond(0.0),
          _duration(0.0),
          _playAnimationForward(true) {}

    AnimEvaluator(const aiAnimation* pAnim);

    void evaluate(const D32 dt, Bone* skeleton);
    void save(std::ofstream& file);
    void load(std::ifstream& file);
    I32 frameIndexAt(const D32 elapsedTime) const;

    inline U32 frameCount() const {
        return to_uint(_transforms.size());
    }

    inline vectorImpl<vectorImpl<mat4<F32>>>& transforms() {
        return _transforms;
    }
    
    inline const vectorImpl<vectorImpl<mat4<F32>>>& transforms() const {
        return _transforms;
    }

    inline vectorImpl<mat4<F32>>& transforms(const U32 frameIndex) {
        assert(frameIndex < to_uint(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline const vectorImpl<mat4<F32>>& transforms(const U32 frameIndex) const {
        assert(frameIndex < to_uint(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline vectorImpl<mat4<F32>>& transforms(const D32 elapsedTime,
                                             I32& resultingFrameIndex) {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_uint(resultingFrameIndex));
    }

    inline vectorImpl<mat4<F32>>& transforms(const D32 elapsedTime) {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    inline const vectorImpl<mat4<F32>>& transforms(const D32 elapsedTime, 
                                                   I32& resultingFrameIndex) const {
        resultingFrameIndex = frameIndexAt(elapsedTime);
        return transforms(to_uint(resultingFrameIndex));
    }

    inline const vectorImpl<mat4<F32>>& transforms(const D32 elapsedTime) const {
        I32 resultingFrameIndex = 0;
        return transforms(elapsedTime, resultingFrameIndex);
    }

    inline void playAnimationForward(bool state) {
        _playAnimationForward = state;
    }

    inline bool playAnimationForward() const {
        return _playAnimationForward;
    }

    inline void ticksPerSecond(D32 tickCount) {
        _ticksPerSecond = tickCount;
    }

    inline D32 ticksPerSecond() const {
        return _ticksPerSecond;
    }

    inline const stringImpl& name() const {
        return _name;
    }

    inline D32 duration() const {
        return _duration;
    }

   protected:
    stringImpl _name;
    /// Array to return transformations results inside.
    vectorImpl<vectorImpl<mat4<F32>>> _transforms;
    /// play forward == true, play backward == false
    bool _playAnimationForward;
    D32 _lastTime;
    D32 _duration;
    D32 _ticksPerSecond;

   private:
    vectorImpl<vec3<U32>> _lastPositions;
    /// vector that holds all bone channels
    vectorImpl<AnimationChannel> _channels;
};

};  // namespace Divide

#endif
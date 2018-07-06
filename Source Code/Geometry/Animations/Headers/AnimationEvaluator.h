/*
   Copyright (c) 2015 DIVIDE-Studio
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
    vectorImpl<aiVectorKey > _positionKeys;
    vectorImpl<aiQuatKey   > _rotationKeys;
    vectorImpl<aiVectorKey > _scalingKeys;
    /** The number of position keys */
    U32 _numPositionKeys;
    U32 _numRotationKeys;
    U32 _numScalingKeys;
};


class AnimEvaluator{
public:

    AnimEvaluator() : _lastTime(0.0),
                      _ticksPerSecond(0.0),
                      _duration(0.0),
                      _playAnimationForward(true)
    {
    }

    AnimEvaluator( const aiAnimation* pAnim);

    void Evaluate(const D32 dt, hashMapImpl<stringImpl, Bone*>& bones);
    void Save(std::ofstream& file);
    void Load(std::ifstream& file);
    I32 GetFrameIndexAt(const D32 elapsedTime) const;

    inline U32 GetFrameCount() const {return (U32)_transforms.size();}

    inline vectorImpl<mat4<F32> >& GetTransforms(const U32 frameIndex) {
        assert(frameIndex < static_cast<I32>(_transforms.size()));
        return _transforms[frameIndex];
    }

    inline vectorImpl<mat4<F32> >& GetTransforms(const D32 elapsedTime) {
        return GetTransforms(static_cast<U32>(GetFrameIndexAt(elapsedTime)));
    }

    inline const vectorImpl<mat4<F32> >& GetTransformsConst(const U32 frameIndex) const {
         assert(frameIndex < static_cast<I32>(_transforms.size()));
         return _transforms[frameIndex];
    }

    inline const vectorImpl<mat4<F32> >& GetTransformsConst(const D32 elapsedTime) const {
        return GetTransformsConst(static_cast<U32>(GetFrameIndexAt(elapsedTime)));
    }

protected:
    friend class SceneAnimator;
    stringImpl _name;

    /// Array to return transformations results inside.
    vectorImpl<vectorImpl<mat4<F32> >> _transforms;
    vectorImpl<vectorImpl<mat4<F32> >> _quatTransforms;

    /// play forward == true, play backward == false
    bool _playAnimationForward;
    D32 _lastTime;
    D32 _duration;
    D32 _ticksPerSecond;

private:

    vectorImpl<vec3<U32> > _lastPositions;
    ///vector that holds all bone channels
    vectorImpl<AnimationChannel> _channels;
};

}; //namespace Divide

#endif
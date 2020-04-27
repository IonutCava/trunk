#include "stdafx.h"

#include "config.h"

#include "Headers/AnimationEvaluator.h"
#include "Headers/AnimationUtils.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Buffers/ShaderBuffer/Headers/ShaderBuffer.h"

#include "Core/Headers/StringHelper.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

AnimEvaluator::AnimEvaluator() : _lastTime(0.0),
                                 _ticksPerSecond(0.0),
                                 _duration(0.0),
                                 _playAnimationForward(true),
                                 _boneTransformBuffer(nullptr)
{
}

// ------------------------------------------------------------------------------------------------
// Constructor on a given animation.
AnimEvaluator::AnimEvaluator(const aiAnimation* pAnim, U32 idx) : AnimEvaluator() {
    _lastTime = 0.0;
    _ticksPerSecond = !IS_ZERO(pAnim->mTicksPerSecond)
                          ? pAnim->mTicksPerSecond
                          : ANIMATION_TICKS_PER_SECOND;
    _duration = pAnim->mDuration;
    _name = pAnim->mName.data;
    if (pAnim->mName.length == 0) {
        _name = Util::StringFormat("unnamed_anim_%d", idx);
    }
    Console::d_printfn(Locale::get(_ID("CREATE_ANIMATION_BEGIN")), _name.c_str());

    _channels.resize(pAnim->mNumChannels);
    for (U32 a = 0; a < pAnim->mNumChannels; a++) {
        aiNodeAnim* srcChannel = pAnim->mChannels[a];
        AnimationChannel& dstChannel = _channels[a];

        dstChannel._name = srcChannel->mNodeName.data;
        dstChannel._nameKey = _ID(dstChannel._name.c_str());

        for (U32 i(0); i < srcChannel->mNumPositionKeys; i++) {
            dstChannel._positionKeys.push_back(srcChannel->mPositionKeys[i]);
        }
        for (U32 i(0); i < srcChannel->mNumRotationKeys; i++) {
            dstChannel._rotationKeys.push_back(srcChannel->mRotationKeys[i]);
        }
        for (U32 i(0); i < srcChannel->mNumScalingKeys; i++) {
            dstChannel._scalingKeys.push_back(srcChannel->mScalingKeys[i]);
        }
        dstChannel._numPositionKeys = srcChannel->mNumPositionKeys;
        dstChannel._numRotationKeys = srcChannel->mNumRotationKeys;
        dstChannel._numScalingKeys = srcChannel->mNumScalingKeys;
    }

    _lastPositions.resize(pAnim->mNumChannels, vec3<U32>());

    Console::d_printfn(Locale::get(_ID("CREATE_ANIMATION_END")), _name.c_str());
}

AnimEvaluator::~AnimEvaluator()
{
}

bool AnimEvaluator::initBuffers(GFXDevice& context) {
    DIVIDE_ASSERT(_boneTransformBuffer == nullptr && !_transforms.empty(),
                  "AnimEvaluator error: can't create bone buffer at current stage!");

    DIVIDE_ASSERT(_transforms.size() <= Config::MAX_BONE_COUNT_PER_NODE,
        "AnimEvaluator error: Too many bones for current node! "
        "Increase MAX_BONE_COUNT_PER_NODE in Config!");

    size_t boneCount = _transforms.front().size();
    U32 numberOfFrames = frameCount();

    vectorEASTL<std::array<mat4<F32>, Config::MAX_BONE_COUNT_PER_NODE>> animationData;
    animationData.resize(numberOfFrames, {MAT4_IDENTITY});

    for (U32 i = 0; i < numberOfFrames; ++i) {
        std::array<mat4<F32>, Config::MAX_BONE_COUNT_PER_NODE>& anim = animationData[i];
        const vectorEASTL<mat4<F32>>& frameTransforms = _transforms[i];
        size_t numberOfTransforms = frameTransforms.size();
        for (U32 j = 0; j < numberOfTransforms; ++j) {
            anim[j].set(frameTransforms[j]);
        }
    }

    ShaderBufferDescriptor bufferDescriptor = {};
    bufferDescriptor._usage = ShaderBuffer::Usage::CONSTANT_BUFFER;
    bufferDescriptor._elementCount = frameCount();
    bufferDescriptor._elementSize = sizeof(mat4<F32>) * Config::MAX_BONE_COUNT_PER_NODE;
    bufferDescriptor._ringBufferLength = 1;
    bufferDescriptor._flags = to_U32(ShaderBuffer::Flags::ALLOW_THREADED_WRITES);
    bufferDescriptor._initialData = animationData.data();
    bufferDescriptor._updateFrequency = BufferUpdateFrequency::ONCE;
    bufferDescriptor._updateUsage = BufferUpdateUsage::CPU_W_GPU_R;
    bufferDescriptor._name = Util::StringFormat("BONE_BUFFER_%d_BONES", boneCount);

    _boneTransformBuffer = context.newSB(bufferDescriptor);

    return numberOfFrames > 0;
}

I32 AnimEvaluator::frameIndexAt(const D64 elapsedTime) const {
    D64 time = 0.0f;
    if (_duration > 0.0) {
        // get a [0.f ... 1.f) value by allowing the percent to wrap around 1
        time = fmod(elapsedTime * _ticksPerSecond, _duration);
    }

    D64 percent = time / _duration;

    // this will invert the percent so the animation plays backwards
    if (!_playAnimationForward) {
        percent = (percent - 1.0) * -1.0;
    }

    return std::min(to_I32(_transforms.size() * percent),
                    to_I32(_transforms.size() - 1));
}

// ------------------------------------------------------------------------------------------------
// Evaluates the animation tracks for a given time stamp.
void AnimEvaluator::evaluate(const D64 dt, Bone* skeleton) {
    D64 pTime = dt * _ticksPerSecond;

    D64 time = 0.0f;
    if (_duration > 0.0) {
        time = fmod(pTime, _duration);
    }

    frameIndexAt(pTime);
    aiVector3D presentPosition(0, 0, 0);
    aiQuaternion presentRotation(1, 0, 0, 0);
    aiQuaternion presentRotationDefault(1, 0, 0, 0);
    aiVector3D presentScaling(1, 1, 1);
    
    // calculate the transformations for each animation channel
    for (U32 a = 0; a < _channels.size(); a++) {
        
        const AnimationChannel* channel = &_channels[a];
        Bone* bonenode = skeleton->find(channel->_nameKey);

        if (bonenode == nullptr) {
            Console::d_errorfn(Locale::get(_ID("ERROR_BONE_FIND")),
                               channel->_name.c_str());
            continue;
        }

        // ******** Position *****
        if (channel->_positionKeys.size() > 0) {
            // Look for present frame number. Search from last position if time
            // is after the last time, else from beginning
            // Should be much quicker than always looking from start for the
            // average use case.
            U32 frame = (time >= _lastTime) ? _lastPositions[a].x : 0;
            while (frame < channel->_positionKeys.size() - 1) {
                if (time < channel->_positionKeys[frame + 1].mTime) {
                    break;
                }
                frame++;
            }

            // interpolate between this frame's value and next frame's value
            U32 nextFrame = (frame + 1) % channel->_positionKeys.size();

            const aiVectorKey& key = channel->_positionKeys[frame];
            const aiVectorKey& nextKey = channel->_positionKeys[nextFrame];
            D64 diffTime = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) diffTime += _duration;
            if (diffTime > 0) {
                F32 factor = F32((time - key.mTime) / diffTime);
                presentPosition =
                    key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else {
                presentPosition = key.mValue;
            }
            _lastPositions[a].x = frame;
        } else {
            presentPosition.Set(0.0f, 0.0f, 0.0f);
        }

        // ******** Rotation *********
        if (channel->_rotationKeys.size() > 0) {
            U32 frame = (time >= _lastTime) ? _lastPositions[a].y : 0;
            while (frame < channel->_rotationKeys.size() - 1) {
                if (time < channel->_rotationKeys[frame + 1].mTime) break;
                frame++;
            }

            // interpolate between this frame's value and next frame's value
            U32 nextFrame = (frame + 1) % channel->_rotationKeys.size();

            const aiQuatKey& key = channel->_rotationKeys[frame];
            const aiQuatKey& nextKey = channel->_rotationKeys[nextFrame];
            D64 diffTime = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) diffTime += _duration;
            if (diffTime > 0) {
                F32 factor = F32((time - key.mTime) / diffTime);
                presentRotation = presentRotationDefault;
                aiQuaternion::Interpolate(presentRotation, key.mValue,
                                          nextKey.mValue, factor);
            } else {
                presentRotation = key.mValue;
            }
            _lastPositions[a].y = frame;
        } else {
            presentRotation = presentRotationDefault;
        }

        // ******** Scaling **********
        if (channel->_scalingKeys.size() > 0) {
            U32 frame = (time >= _lastTime) ? _lastPositions[a].z : 0;
            while (frame < channel->_scalingKeys.size() - 1) {
                if (time < channel->_scalingKeys[frame + 1].mTime) break;
                frame++;
            }

            presentScaling = channel->_scalingKeys[frame].mValue;
            _lastPositions[a].z = frame;
        } else {
            presentScaling.Set(1.0f, 1.0f, 1.0f);
        }

        aiMatrix4x4 mat(presentRotation.GetMatrix());
        mat.a1 *= presentScaling.x;
        mat.b1 *= presentScaling.x;
        mat.c1 *= presentScaling.x;
        mat.a2 *= presentScaling.y;
        mat.b2 *= presentScaling.y;
        mat.c2 *= presentScaling.y;
        mat.a3 *= presentScaling.z;
        mat.b3 *= presentScaling.z;
        mat.c3 *= presentScaling.z;
        mat.a4  = presentPosition.x;
        mat.b4  = presentPosition.y;
        mat.c4  = presentPosition.z;
        AnimUtils::TransformMatrix(mat, bonenode->_localTransform);
    }
    _lastTime = time;
}
};
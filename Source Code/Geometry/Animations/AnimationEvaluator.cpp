#include "Headers/AnimationEvaluator.h"
#include "Headers/AnimationUtils.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

AnimEvaluator::AnimEvaluator() : _lastTime(0.0),
                                 _ticksPerSecond(0.0),
                                 _duration(0.0),
                                 _playAnimationForward(true)
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
    if (_name.empty()) {
        _name = Util::StringFormat("unnamed_anim_%d", idx);
    }
    Console::d_printfn(Locale::get("CREATE_ANIMATION_BEGIN"), _name.c_str());

    _channels.resize(pAnim->mNumChannels);
    for (U32 a = 0; a < pAnim->mNumChannels; a++) {
        aiNodeAnim* srcChannel = pAnim->mChannels[a];
        AnimationChannel& dstChannel = _channels[a];

        dstChannel._name = srcChannel->mNodeName.data;
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

    Console::d_printfn(Locale::get("CREATE_ANIMATION_END"), _name.c_str());
}

AnimEvaluator::~AnimEvaluator()
{
}

bool AnimEvaluator::initBuffers() {
    DIVIDE_ASSERT(_boneTransformBuffer.get() == nullptr && !_transforms.empty(),
                  "AnimEvaluator error: can't create bone buffer at current stage!");

    _boneTransformBuffer.reset(GFX_DEVICE.newSB("dvd_BoneTransforms", 1, true, false, BufferUpdateFrequency::ONCE));

    DIVIDE_ASSERT(_transforms.size() <= Config::MAX_BONE_COUNT_PER_NODE,
        "AnimEvaluator error: Too many bones for current node! "
        "Increase MAX_BONE_COUNT_PER_NODE in Config!");
    size_t paddingFactor = _boneTransformBuffer->getAlignmentRequirement();

    U32 bonePadding = 0;
    size_t boneCount = _transforms.front().size();
    size_t bufferSize = sizeof(mat4<F32>) * boneCount;
    while (bufferSize % paddingFactor != 0) {
        bufferSize += sizeof(mat4<F32>);
        bonePadding++;
    }

    _boneTransformBuffer->Create(frameCount(), bufferSize);

    vectorImpl<mat4<F32>> animationData;
    animationData.reserve((boneCount + bonePadding) * frameCount());

    for (const vectorImpl<mat4<F32>>& frameTransforms : _transforms) {
        animationData.insert(std::cend(animationData), std::cbegin(frameTransforms), std::cend(frameTransforms));
        for (U32 i = 0; i < bonePadding; ++i) {
            animationData.push_back(mat4<F32>());
        }
    }

    vectorAlg::shrinkToFit(animationData);

    _boneTransformBuffer->SetData((const bufferPtr)animationData.data());

    return !animationData.empty();
}

I32 AnimEvaluator::frameIndexAt(const D32 elapsedTime) const {
    D32 time = 0.0f;
    if (_duration > 0.0) {
        // get a [0.f ... 1.f) value by allowing the percent to wrap around 1
        time = fmod(elapsedTime * _ticksPerSecond, _duration);
    }

    D32 percent = time / _duration;

    // this will invert the percent so the animation plays backwards
    if (!_playAnimationForward) {
        percent = (percent - 1.0) * -1.0;
    }

    return std::min(to_int(_transforms.size() * percent),
                    to_int(_transforms.size() - 1));
}

// ------------------------------------------------------------------------------------------------
// Evaluates the animation tracks for a given time stamp.
void AnimEvaluator::evaluate(const D32 dt, Bone* skeleton) {
    D32 pTime = dt * _ticksPerSecond;

    D32 time = 0.0f;
    if (_duration > 0.0) {
        time = fmod(pTime, _duration);
    }

    frameIndexAt(pTime);
    // calculate the transformations for each animation channel
    for (U32 a = 0; a < _channels.size(); a++) {
        const AnimationChannel* channel = &_channels[a];
        Bone* bonenode = skeleton->find(channel->_name);

        if (bonenode == nullptr) {
            Console::d_errorfn(Locale::get("ERROR_BONE_FIND"),
                               channel->_name.c_str());
            continue;
        }

        // ******** Position *****
        aiVector3D presentPosition(0, 0, 0);
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
            D32 diffTime = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) diffTime += _duration;
            if (diffTime > 0) {
                F32 factor = F32((time - key.mTime) / diffTime);
                presentPosition =
                    key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else {
                presentPosition = key.mValue;
            }
            _lastPositions[a].x = frame;
        }
        // ******** Rotation *********
        aiQuaternion presentRotation(1, 0, 0, 0);
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
            D32 diffTime = nextKey.mTime - key.mTime;
            if (diffTime < 0.0) diffTime += _duration;
            if (diffTime > 0) {
                F32 factor = F32((time - key.mTime) / diffTime);
                aiQuaternion::Interpolate(presentRotation, key.mValue,
                                          nextKey.mValue, factor);
            } else
                presentRotation = key.mValue;
            _lastPositions[a].y = frame;
        }

        // ******** Scaling **********
        aiVector3D presentScaling(1, 1, 1);
        if (channel->_scalingKeys.size() > 0) {
            U32 frame = (time >= _lastTime) ? _lastPositions[a].z : 0;
            while (frame < channel->_scalingKeys.size() - 1) {
                if (time < channel->_scalingKeys[frame + 1].mTime) break;
                frame++;
            }

            presentScaling = channel->_scalingKeys[frame].mValue;
            _lastPositions[a].z = frame;
        }

        aiMatrix4x4& mat = bonenode->_localTransform;
        mat = aiMatrix4x4(presentRotation.GetMatrix());
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
        if (GFX_DEVICE.getAPI() == GFXDevice::RenderAPI::Direct3D) {
            mat.Transpose();
        }
    }
    _lastTime = time;
}
};
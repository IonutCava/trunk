#include "Headers/AnimationEvaluator.h"
#include "Headers/AnimationUtils.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {

// ------------------------------------------------------------------------------------------------
// Constructor on a given animation.
AnimEvaluator::AnimEvaluator( const aiAnimation* pAnim) {
    _lastTime = 0.0;
    _ticksPerSecond = pAnim->mTicksPerSecond != 0.0f ? pAnim->mTicksPerSecond : ANIMATION_TICKS_PER_SECOND;
    _duration = pAnim->mDuration;
    _name = pAnim->mName.data;

    D_PRINT_FN(Locale::get("CREATE_ANIMATION_BEGIN"),_name.c_str());

    _channels.resize(pAnim->mNumChannels);
    for( U32 a = 0; a < pAnim->mNumChannels; a++){
        _channels[a]._name = pAnim->mChannels[a]->mNodeName.data;
        for (U32 i(0); i < pAnim->mChannels[a]->mNumPositionKeys; i++) {
            _channels[a]._positionKeys.push_back(pAnim->mChannels[a]->mPositionKeys[i]);
        }
        for (U32 i(0); i < pAnim->mChannels[a]->mNumRotationKeys; i++) {
            _channels[a]._rotationKeys.push_back(pAnim->mChannels[a]->mRotationKeys[i]);
        }
        for (U32 i(0); i < pAnim->mChannels[a]->mNumScalingKeys; i++) {
            _channels[a]._scalingKeys.push_back(pAnim->mChannels[a]->mScalingKeys[i]);
        }
        _channels[a]._numPositionKeys = pAnim->mChannels[a]->mNumPositionKeys;
        _channels[a]._numRotationKeys = pAnim->mChannels[a]->mNumRotationKeys;
        _channels[a]._numScalingKeys  = pAnim->mChannels[a]->mNumScalingKeys;
    }

    _lastPositions.resize( pAnim->mNumChannels, vec3<U32>());

    D_PRINT_FN(Locale::get("CREATE_ANIMATION_END"), _name.c_str());
}

I32 AnimEvaluator::GetFrameIndexAt(const D32 elapsedTime){
    // get a [0.f ... 1.f) value by allowing the percent to wrap around 1
    D32 ptime = elapsedTime * _ticksPerSecond;

    D32 time = 0.0f;
    if( _duration > 0.0)
        time = fmod( ptime, _duration);

    F32 percent = time / _duration;

    if(!_playAnimationForward)
        percent= (percent-1.0f)*-1.0f;// this will invert the percent so the animation plays backwards

    _lastFrameIndex = std::min(static_cast<I32>(_transforms.size() * percent), static_cast<I32>(_transforms.size() - 1));

    return _lastFrameIndex;
}

// ------------------------------------------------------------------------------------------------
// Evaluates the animation tracks for a given time stamp.
void AnimEvaluator::Evaluate(const D32 dt, hashMapImpl<stringImpl, Bone*>& bones) {
    D32 pTime = dt * _ticksPerSecond;

    D32 time = 0.0f;
    if( _duration > 0.0)
        time = fmod( pTime, _duration);
    GetFrameIndexAt(pTime);
    hashMapImpl<stringImpl, Bone*>::iterator bonenode;
    // calculate the transformations for each animation channel
    for( U32 a = 0; a < _channels.size(); a++){
        const AnimationChannel* channel = &_channels[a];
        bonenode = bones.find(channel->_name);

        if(bonenode == bones.end()) {
            D_ERROR_FN(Locale::get("ERROR_BONE_FIND"),channel->_name.c_str());
            continue;
        }

        // ******** Position *****
        aiVector3D presentPosition( 0, 0, 0);
        if( channel->_positionKeys.size() > 0){
            // Look for present frame number. Search from last position if time is after the last time, else from beginning
            // Should be much quicker than always looking from start for the average use case.
            U32 frame = (time >= _lastTime) ? _lastPositions[a].x: 0;
            while( frame < channel->_positionKeys.size() - 1){
                if( time < channel->_positionKeys[frame+1].mTime){
                    break;
                }
                frame++;
            }

            // interpolate between this frame's value and next frame's value
            U32 nextFrame = (frame + 1) % channel->_positionKeys.size();

            const aiVectorKey& key = channel->_positionKeys[frame];
            const aiVectorKey& nextKey = channel->_positionKeys[nextFrame];
            D32 diffTime = nextKey.mTime - key.mTime;
            if( diffTime < 0.0)
                diffTime += _duration;
            if( diffTime > 0) {
                F32 factor = F32( (time - key.mTime) / diffTime);
                presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
            } else {
                presentPosition = key.mValue;
            }
            _lastPositions[a].x = frame;
        }
        // ******** Rotation *********
        aiQuaternion presentRotation( 1, 0, 0, 0);
        if( channel->_rotationKeys.size() > 0)
        {
            U32 frame = (time >= _lastTime) ? _lastPositions[a].y : 0;
            while( frame < channel->_rotationKeys.size()  - 1){
                if( time < channel->_rotationKeys[frame+1].mTime)
                    break;
                frame++;
            }

            // interpolate between this frame's value and next frame's value
            U32 nextFrame = (frame + 1) % channel->_rotationKeys.size() ;

            const aiQuatKey& key = channel->_rotationKeys[frame];
            const aiQuatKey& nextKey = channel->_rotationKeys[nextFrame];
            D32 diffTime = nextKey.mTime - key.mTime;
            if( diffTime < 0.0) diffTime += _duration;
            if( diffTime > 0) {
                F32 factor = F32( (time - key.mTime) / diffTime);
                aiQuaternion::Interpolate( presentRotation, key.mValue, nextKey.mValue, factor);
            } else presentRotation = key.mValue;
            _lastPositions[a].y = frame;
        }

        // ******** Scaling **********
        aiVector3D presentScaling( 1, 1, 1);
        if( channel->_scalingKeys.size() > 0) {
            U32 frame = (time >= _lastTime) ? _lastPositions[a].z : 0;
            while( frame < channel->_scalingKeys.size() - 1){
                if( time < channel->_scalingKeys[frame+1].mTime)
                    break;
                frame++;
            }

            presentScaling = channel->_scalingKeys[frame].mValue;
            _lastPositions[a].z = frame;
        }

        aiMatrix4x4 mat = aiMatrix4x4( presentRotation.GetMatrix());

        mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
        mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
        mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
        mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
        if(GFX_DEVICE.getApi() == Direct3D){
            mat.Transpose();
        }

        bonenode->second->_localTransform = mat;
    }
    _lastTime = time;
}

};
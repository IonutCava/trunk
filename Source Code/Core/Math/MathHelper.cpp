#include "Headers/MathClasses.h"
#include "Headers/Quaternion.h"

namespace Divide {
namespace Util {
    void normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw, bool normPitch, bool normRoll) {
        if(normYaw)
        {
            F32 yaw = degrees ? RADIANS(inputRotation.yaw) : inputRotation.yaw;
            if(yaw < -M_PI)
            {
                yaw = fmod(yaw, M_PI * 2.0f);
                if(yaw < -M_PI)
                {
                    yaw += M_PI * 2.0f;
                }
                inputRotation.yaw = DEGREES(yaw);
            }
            else if(yaw >M_PI)
            {
                yaw = fmod(yaw, M_PI * 2.0f);
                if(yaw > M_PI)
                {
                    yaw -= M_PI * 2.0f;
                }
                inputRotation.yaw = degrees ? DEGREES(yaw) : yaw;
            }
        }
        if(normPitch)
        {
            F32 pitch = degrees ? RADIANS(inputRotation.pitch) : inputRotation.pitch;
            if(pitch < -M_PI)
            {
                pitch = fmod(pitch, M_PI * 2.0f);
                if(pitch < -M_PI)
                {
                    pitch += M_PI * 2.0f;
                }
                inputRotation.pitch = DEGREES(pitch);
            }
            else if(pitch > M_PI)
            {
                pitch = fmod(pitch,M_PI * 2.0f);
                if(pitch > M_PI)
                {
                    pitch -= M_PI * 2.0f;
                }
                inputRotation.pitch = degrees ? DEGREES(pitch) : pitch;
            }
        }
        if(normRoll)
        {
            F32 roll= degrees ? RADIANS(inputRotation.roll) : inputRotation.roll;
            if(roll < -M_PI)
            {
                roll = fmod(roll, M_PI * 2.0f);
                if(roll < -M_PI)
                {
                    roll += M_PI * 2.0f;
                }
                inputRotation.roll = DEGREES(roll);
            }
            else if(roll > M_PI)
            {
                roll = fmod(roll, M_PI * 2.0f);
                if(roll > M_PI)
                {
                    roll -= M_PI * 2.0f;
                }
                inputRotation.roll = degrees ? DEGREES(roll) : roll;
            }
        }
    }
}
}; //namespace Divide
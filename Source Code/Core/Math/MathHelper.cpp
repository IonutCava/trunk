#include "Headers/MathClasses.h"
#include "Headers/Quaternion.h"

namespace Divide {
namespace Util {
    vec4<U8> toByteColor(const vec4<F32>& floatColor) {
        return vec4<U8>(static_cast<U8>(floatColor.r * 255),
                        static_cast<U8>(floatColor.g * 255), 
                        static_cast<U8>(floatColor.b * 255),
                        static_cast<U8>(floatColor.a * 255));
    }

    vec3<U8> toByteColor(const vec3<F32>& floatColor) {
        return vec3<U8>(static_cast<U8>(floatColor.r * 255), 
                        static_cast<U8>(floatColor.g * 255), 
                        static_cast<U8>(floatColor.b * 255));
    }

    vec4<F32> toFloatColor(const vec4<U8>& byteColor) {
        return vec4<F32>(static_cast<F32>(byteColor.r / 255),
                         static_cast<F32>(byteColor.g / 255), 
                         static_cast<F32>(byteColor.b / 255), 
                         static_cast<F32>(byteColor.a / 255));
    }

    vec3<F32> toFloatColor(const vec3<U8>& byteColor) {
        return vec3<F32>(static_cast<F32>(byteColor.r / 255), 
                         static_cast<F32>(byteColor.g / 255), 
                         static_cast<F32>(byteColor.b / 255));
    }

    void normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw, bool normPitch, bool normRoll) {
        if (normYaw) {
            F32 yaw = degrees ? RADIANS(inputRotation.yaw) : inputRotation.yaw;
            if (yaw < -M_PI) {
                yaw = fmod(yaw, M_PI * 2.0f);
                if (yaw < -M_PI) {
                    yaw += M_PI * 2.0f;
                }
                inputRotation.yaw = DEGREES(yaw);
            } else if(yaw >M_PI) {
                yaw = fmod(yaw, M_PI * 2.0f);
                if (yaw > M_PI) {
                    yaw -= M_PI * 2.0f;
                }
                inputRotation.yaw = degrees ? DEGREES(yaw) : yaw;
            }
        }
        if (normPitch) {
            F32 pitch = degrees ? RADIANS(inputRotation.pitch) : inputRotation.pitch;
            if (pitch < -M_PI) {
                pitch = fmod(pitch, M_PI * 2.0f);
                if(pitch < -M_PI)
                {
                    pitch += M_PI * 2.0f;
                }
                inputRotation.pitch = DEGREES(pitch);
            } else if(pitch > M_PI) {
                pitch = fmod(pitch,M_PI * 2.0f);
                if (pitch > M_PI) {
                    pitch -= M_PI * 2.0f;
                }
                inputRotation.pitch = degrees ? DEGREES(pitch) : pitch;
            }
        }
        if(normRoll) {
            F32 roll= degrees ? RADIANS(inputRotation.roll) : inputRotation.roll;
            if (roll < -M_PI) {
                roll = fmod(roll, M_PI * 2.0f);
                if(roll < -M_PI)
                {
                    roll += M_PI * 2.0f;
                }
                inputRotation.roll = DEGREES(roll);
            } else if(roll > M_PI) {
                roll = fmod(roll, M_PI * 2.0f);
                if (roll > M_PI) {
                    roll -= M_PI * 2.0f;
                }
                inputRotation.roll = degrees ? DEGREES(roll) : roll;
            }
        }
    }
}
}; //namespace Divide
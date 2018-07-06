#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

namespace Divide {
namespace Util {
    void permute(char* input,
        U32 startingIndex,
        U32 stringLength,
        vectorImpl<stringImpl>& container) {
        if (startingIndex == stringLength - 1) {
            container.push_back(input);
        }
        else {
            for (U32 i = startingIndex; i < stringLength; i++) {
                swap(&input[startingIndex], &input[i]);
                permute(input, startingIndex + 1, stringLength, container);
                swap(&input[startingIndex], &input[i]);
            }
        }
    }

    void getPermutations(const stringImpl& inputString,
        vectorImpl<stringImpl>& permutationContainer) {
        permutationContainer.clear();
        permute((char*)inputString.c_str(), 0, (U32)inputString.length() - 1, permutationContainer);
    }

    bool isNumber(const stringImpl& s) {
        _ssBuffer.str("");
        _ssBuffer << s.c_str();
        F32 number;
        _ssBuffer >> number;
        if (_ssBuffer.good()) {
            return false;
        }
        if (number == 0 && s[0] != 0) {
            return false;
        }
        return true;
    }


    /// http://stackoverflow.com/questions/53849/how-do-i-tokenize-a-string-in-c
    void split(const stringImpl& input,
        const char* delimiter,
        vectorImpl<stringImpl>& outputVector) {
        stringAlg::stringSize delLen = static_cast<stringAlg::stringSize>(strlen(delimiter));
        assert(!input.empty() && delLen > 0);

        stringAlg::stringSize start = 0, end = 0;
        while (end != stringImpl::npos) {
            end = input.find(delimiter, start);
            // If at end, use length=maxLength.  Else use length=end-start.
            outputVector.push_back(input.substr(start,
                (end == stringImpl::npos) ? stringImpl::npos :
                end - start));
            // If at end, use start=maxSize.  Else use start=end+delimiter.
            start = ((end > (stringImpl::npos - delLen)) ? stringImpl::npos : end + delLen);
        }
    }

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
            F32 yaw = degrees ? Angle::DegreesToRadians(inputRotation.yaw) : inputRotation.yaw;
            if (yaw < -M_PI) {
                yaw = fmod(yaw, M_PI * 2.0f);
                if (yaw < -M_PI) {
                    yaw += M_PI * 2.0f;
                }
                inputRotation.yaw = Angle::RadiansToDegrees(yaw);
            } else if (yaw > M_PI) {
                yaw = fmod(yaw, M_PI * 2.0f);
                if (yaw > M_PI) {
                    yaw -= M_PI * 2.0f;
                }
                inputRotation.yaw = degrees ? Angle::RadiansToDegrees(yaw) : yaw;
            }
        }
        if (normPitch) {
            F32 pitch = degrees ? Angle::DegreesToRadians(inputRotation.pitch) : inputRotation.pitch;
            if (pitch < -M_PI) {
                pitch = fmod(pitch, M_PI * 2.0f);
                if (pitch < -M_PI) {
                    pitch += M_PI * 2.0f;
                }
                inputRotation.pitch = Angle::RadiansToDegrees(pitch);
            } else if (pitch > M_PI) {
                pitch = fmod(pitch, M_PI * 2.0f);
                if (pitch > M_PI) {
                    pitch -= M_PI * 2.0f;
                }
                inputRotation.pitch = degrees ? Angle::RadiansToDegrees(pitch) : pitch;
            }
        }
        if (normRoll) {
            F32 roll = degrees ? Angle::DegreesToRadians(inputRotation.roll) : inputRotation.roll;
            if (roll < -M_PI) {
                roll = fmod(roll, M_PI * 2.0f);
                if (roll < -M_PI) {
                    roll += M_PI * 2.0f;
                }
                inputRotation.roll = Angle::RadiansToDegrees(roll);
            } else if (roll > M_PI) {
                roll = fmod(roll, M_PI * 2.0f);
                if (roll > M_PI) {
                    roll -= M_PI * 2.0f;
                }
                inputRotation.roll = degrees ? Angle::RadiansToDegrees(roll) : roll;
            }
        }
    }
};//namespace Util
}; //namespace Divide
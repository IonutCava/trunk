#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

namespace Divide {
namespace Util {

static boost::thread_specific_ptr<vectorImpl<GlobalFloatEvent>> _globalFloatEvents;

void getPermutations(const stringImpl& inputString,
                     vectorImpl<stringImpl>& permutationContainer) {
    permutationContainer.clear();
    std::string tempCpy(stringAlg::fromBase(inputString));
    std::sort(std::begin(tempCpy), std::end(tempCpy));
    do {
        permutationContainer.push_back(inputString);
    } while (std::next_permutation(std::begin(tempCpy), std::end(tempCpy)));
}

bool isNumber(const stringImpl& s) {
    F32 number;
    if (std::istringstream(s.c_str()) >> number) {
        return !(number == 0 && s[0] != 0);
    }
    return false;
}

vectorImpl<stringImpl>& split(const stringImpl& input, char delimiter,
                              vectorImpl<stringImpl>& elems) {
    std::stringstream ss(input.c_str());
    std::string item;
    while (std::getline(ss, item, delimiter)) {
        elems.push_back(stringAlg::toBase(item));
    }
    return elems;
}

vectorImpl<stringImpl> split(const stringImpl& input, char delimiter) {
    vectorImpl<stringImpl> elems;
    split(input, delimiter, elems);
    return elems;
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

void normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw,
               bool normPitch, bool normRoll) {
    if (normYaw) {
        F32 yaw = degrees ? Angle::DegreesToRadians(inputRotation.yaw)
                          : inputRotation.yaw;
        if (yaw < -M_PI) {
            yaw = fmod(yaw, M_PI * 2.0f);
            if (yaw < -M_PI) {
                yaw += static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.yaw = Angle::RadiansToDegrees(yaw);
        } else if (yaw > M_PI) {
            yaw = fmod(yaw, M_PI * 2.0f);
            if (yaw > M_PI) {
                yaw -= static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.yaw = degrees ? Angle::RadiansToDegrees(yaw) : yaw;
        }
    }
    if (normPitch) {
        F32 pitch = degrees ? Angle::DegreesToRadians(inputRotation.pitch)
                            : inputRotation.pitch;
        if (pitch < -M_PI) {
            pitch = fmod(pitch, M_PI * 2.0f);
            if (pitch < -M_PI) {
                pitch += static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.pitch = Angle::RadiansToDegrees(pitch);
        } else if (pitch > M_PI) {
            pitch = fmod(pitch, M_PI * 2.0f);
            if (pitch > M_PI) {
                pitch -= static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.pitch =
                degrees ? Angle::RadiansToDegrees(pitch) : pitch;
        }
    }
    if (normRoll) {
        F32 roll = degrees ? Angle::DegreesToRadians(inputRotation.roll)
                           : inputRotation.roll;
        if (roll < -M_PI) {
            roll = fmod(roll, M_PI * 2.0f);
            if (roll < -M_PI) {
                roll += static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.roll = Angle::RadiansToDegrees(roll);
        } else if (roll > M_PI) {
            roll = fmod(roll, M_PI * 2.0f);
            if (roll > M_PI) {
                roll -= static_cast<F32>(M_PI) * 2.0f;
            }
            inputRotation.roll = degrees ? Angle::RadiansToDegrees(roll) : roll;
        }
    }
}

void flushFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vec->clear();
}

void recordFloatEvent(const char* eventName, F32 eventValue, U64 timestamp) {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    GlobalFloatEvent floatEvent{ eventName, eventValue, timestamp };
    vec->push_back(floatEvent);
}

const vectorImpl<GlobalFloatEvent>& getFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if (!vec) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }

    return *vec;
}

void plotFloatEvents(const char* eventName,
                     vectorImpl<GlobalFloatEvent> eventsCopy,
                     GraphPlot2D& targetGraph) {
    targetGraph._plotName = eventName;
    targetGraph._coords.clear();
    for (GlobalFloatEvent& crtEvent : eventsCopy) {
        // direct pointer compare
        if (eventName == crtEvent._eventName) {
            vectorAlg::emplace_back(
                targetGraph._coords,
                static_cast<F32>(
                    Time::MicrosecondsToMilliseconds(crtEvent._timestamp)),
                crtEvent._eventValue);
        }
    }
}
};  // namespace Util
};  // namespace Divide
#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

#include <boost/thread/tss.hpp>
#include <cstdarg>

namespace Divide {
namespace Util {

static boost::thread_specific_ptr<vectorImpl<GlobalFloatEvent>> _globalFloatEvents;

void GetPermutations(const stringImpl& inputString,
                     vectorImpl<stringImpl>& permutationContainer) {
    permutationContainer.clear();
    stringImpl tempCpy(inputString);
    std::sort(std::begin(tempCpy), std::end(tempCpy));
    do {
        permutationContainer.push_back(inputString);
    } while (std::next_permutation(std::begin(tempCpy), std::end(tempCpy)));
}

bool IsNumber(const stringImpl& s) {
    F32 number;
    if (std::istringstream(s) >> number) {
        return !(number == 0 && s[0] != 0);
    }
    return false;
}

vectorImpl<stringImpl>& Split(const stringImpl& input, char delimiter,
                              vectorImpl<stringImpl>& elems) {
    std::stringstream ss(input);
    stringImpl item;
    while (std::getline(ss, item, delimiter)) {
        elems.push_back(item);
    }
    return elems;
}

vectorImpl<stringImpl> Split(const stringImpl& input, char delimiter) {
    vectorImpl<stringImpl> elems;
    Split(input, delimiter, elems);
    return elems;
}

stringImpl StringFormat(const stringImpl& fmt_str, ...) {
    // Reserve two times as much as the length of the fmt_str
    I32 final_n, n = to_int(fmt_str.size()) * 2; 
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(true) {
        /// Wrap the plain char array into the unique_ptr
        formatted.reset(new char[n]); 
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n) {
            n += std::abs(final_n - n + 1);
        } else {
            break;
        }
    }

    return formatted.get();
}

vec4<U8> ToByteColor(const vec4<F32>& floatColor) {
    return vec4<U8>(ToByteColor(floatColor.rgb()),
                    static_cast<U8>(floatColor.a * 255));
}

vec3<U8> ToByteColor(const vec3<F32>& floatColor) {
    return vec3<U8>(static_cast<U8>(floatColor.r * 255),
                    static_cast<U8>(floatColor.g * 255),
                    static_cast<U8>(floatColor.b * 255));
}

vec4<U32> ToUIntColor(const vec4<F32>& floatColor) {
    return vec4<U32>(ToUIntColor(floatColor.rgb()),
                     to_uint(floatColor.a * 255));
}

vec3<U32> ToUIntColor(const vec3<F32>& floatColor) {
    vec3<U8> tempColor(ToByteColor(floatColor));
    return vec3<U32>(to_uint(tempColor.r),
                     to_uint(tempColor.g),
                     to_uint(tempColor.b));
}

vec4<F32> ToFloatColor(const vec4<U8>& byteColor) {
    return vec4<F32>(ToFloatColor(byteColor.rgb()),
                     byteColor.a / 255.0f);
}

vec3<F32> ToFloatColor(const vec3<U8>& byteColor) {
    return vec3<F32>(byteColor.r / 255.0f,
                     byteColor.g / 255.0f,
                     byteColor.b / 255.0f);
}

vec4<F32> ToFloatColor(const vec4<U32>& uintColor) {
    return vec4<F32>(ToFloatColor(uintColor.rgb()),
                     uintColor.a / 255.0f);
}

vec3<F32> ToFloatColor(const vec3<U32>& uintColor) {
    return vec3<F32>(uintColor.r / 255.0f,
                     uintColor.g / 255.0f,
                     uintColor.b / 255.0f);
}

F32 PACK_VEC3(const vec3<F32>& value) {
    return PACK_FLOAT(FLOAT_TO_CHAR(value.x),
                      FLOAT_TO_CHAR(value.y),
                      FLOAT_TO_CHAR(value.z));
}

vec3<F32> UNPACK_VEC3(F32 value) {
    vec3<F32> ret;
    UNPACK_FLOAT(value, ret.x, ret.y, ret.z);
    return ret;
}

void Normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw,
               bool normPitch, bool normRoll) {
    if (normYaw) {
        F32 yaw = degrees ? Angle::DegreesToRadians(inputRotation.yaw)
                          : inputRotation.yaw;
        if (yaw < -M_PI) {
            yaw = fmod(yaw, to_float(M_PI) * 2.0f);
            if (yaw < -M_PI) {
                yaw += to_float(M_PI) * 2.0f;
            }
            inputRotation.yaw = Angle::RadiansToDegrees(yaw);
        } else if (yaw > M_PI) {
            yaw = fmod(yaw, to_float(M_PI) * 2.0f);
            if (yaw > M_PI) {
                yaw -= to_float(M_PI) * 2.0f;
            }
            inputRotation.yaw = degrees ? Angle::RadiansToDegrees(yaw) : yaw;
        }
    }
    if (normPitch) {
        F32 pitch = degrees ? Angle::DegreesToRadians(inputRotation.pitch)
                            : inputRotation.pitch;
        if (pitch < -M_PI) {
            pitch = fmod(pitch, to_float(M_PI) * 2.0f);
            if (pitch < -M_PI) {
                pitch += to_float(M_PI) * 2.0f;
            }
            inputRotation.pitch = Angle::RadiansToDegrees(pitch);
        } else if (pitch > M_PI) {
            pitch = fmod(pitch, to_float(M_PI) * 2.0f);
            if (pitch > M_PI) {
                pitch -= to_float(M_PI) * 2.0f;
            }
            inputRotation.pitch =
                degrees ? Angle::RadiansToDegrees(pitch) : pitch;
        }
    }
    if (normRoll) {
        F32 roll = degrees ? Angle::DegreesToRadians(inputRotation.roll)
                           : inputRotation.roll;
        if (roll < -M_PI) {
            roll = fmod(roll, to_float(M_PI) * 2.0f);
            if (roll < -M_PI) {
                roll += to_float(M_PI) * 2.0f;
            }
            inputRotation.roll = Angle::RadiansToDegrees(roll);
        } else if (roll > M_PI) {
            roll = fmod(roll, to_float(M_PI) * 2.0f);
            if (roll > M_PI) {
                roll -= to_float(M_PI) * 2.0f;
            }
            inputRotation.roll = degrees ? Angle::RadiansToDegrees(roll) : roll;
        }
    }
}

void FlushFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vec->clear();
}

void RecordFloatEvent(const stringImpl& eventName, F32 eventValue, U64 timestamp) {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vectorAlg::emplace_back(*vec, eventName, eventValue, timestamp);
}

const vectorImpl<GlobalFloatEvent>& GetFloatEvents() {
    vectorImpl<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if (!vec) {
        vec = new vectorImpl<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }

    return *vec;
}

void PlotFloatEvents(const stringImpl& eventName,
                     vectorImpl<GlobalFloatEvent> eventsCopy,
                     GraphPlot2D& targetGraph) {
    targetGraph._plotName = eventName;
    targetGraph._coords.clear();
    for (GlobalFloatEvent& crtEvent : eventsCopy) {
        if (eventName.compare(crtEvent._eventName) == 0) {
            vectorAlg::emplace_back(
                targetGraph._coords,
                Time::MicrosecondsToMilliseconds<F32>(crtEvent._timeStamp),
                crtEvent._eventValue);
        }
    }
}
};  // namespace Util
};  // namespace Divide

#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

#include <boost/thread/tss.hpp>


namespace Divide {
namespace Util {

static boost::thread_specific_ptr<vectorImpl<GlobalFloatEvent>> _globalFloatEvents;


void ToByteColour(const vec4<F32>& floatColour, vec4<U8>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR(floatColour.r),
                 FLOAT_TO_CHAR(floatColour.g),
                 FLOAT_TO_CHAR(floatColour.b),
                 FLOAT_TO_CHAR(floatColour.a));
}

void ToByteColour(const vec3<F32>& floatColour, vec3<U8>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR_SNORM(floatColour.r),
                  FLOAT_TO_CHAR_SNORM(floatColour.g),
                  FLOAT_TO_CHAR_SNORM(floatColour.b));
}

void ToIntColour(const vec4<F32>& floatColour, vec4<I32>& colourOut) {
    colourOut.set(FLOAT_TO_SCHAR_SNORM(floatColour.r),
                  FLOAT_TO_SCHAR_SNORM(floatColour.g),
                  FLOAT_TO_SCHAR_SNORM(floatColour.b),
                  FLOAT_TO_SCHAR_SNORM(floatColour.a));
}

void ToIntColour(const vec3<F32>& floatColour, vec3<I32>& colourOut) {
    colourOut.set(to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.r)),
                  to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.g)),
                  to_uint(FLOAT_TO_SCHAR_SNORM(floatColour.b)));
}

void ToUIntColour(const vec4<F32>& floatColour, vec4<U32>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR_SNORM(floatColour.r),
                 FLOAT_TO_CHAR_SNORM(floatColour.g),
                 FLOAT_TO_CHAR_SNORM(floatColour.b),
                 FLOAT_TO_CHAR_SNORM(floatColour.a));
}

void ToUIntColour(const vec3<F32>& floatColour, vec3<U32>& colourOut) {
    colourOut.set(to_uint(FLOAT_TO_CHAR_SNORM(floatColour.r)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColour.g)),
                 to_uint(FLOAT_TO_CHAR_SNORM(floatColour.b)));
}

void ToFloatColour(const vec4<U8>& byteColour, vec4<F32>& colourOut) {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                 CHAR_TO_FLOAT_SNORM(byteColour.g),
                 CHAR_TO_FLOAT_SNORM(byteColour.b),
                 CHAR_TO_FLOAT_SNORM(byteColour.a));
}

void ToFloatColour(const vec3<U8>& byteColour, vec3<F32>& colourOut) {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                 CHAR_TO_FLOAT_SNORM(byteColour.g),
                 CHAR_TO_FLOAT_SNORM(byteColour.b));
}

void ToFloatColour(const vec4<U32>& uintColour, vec4<F32>& colourOut) {
    colourOut.set(uintColour.r / 255.0f,
                 uintColour.g / 255.0f,
                 uintColour.b / 255.0f,
                 uintColour.a / 255.0f);
}

void ToFloatColour(const vec3<U32>& uintColour, vec3<F32>& colourOut) {
    colourOut.set(uintColour.r / 255.0f,
                 uintColour.g / 255.0f,
                 uintColour.b / 255.0f);
}

vec4<U8> ToByteColour(const vec4<F32>& floatColour) {
    vec4<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec3<U8>  ToByteColour(const vec3<F32>& floatColour) {
    vec3<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec4<I32> ToIntColour(const vec4<F32>& floatColour) {
    vec4<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<I32> ToIntColour(const vec3<F32>& floatColour) {
    vec3<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<U32> ToUIntColour(const vec4<F32>& floatColour) {
    vec4<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<U32> ToUIntColour(const vec3<F32>& floatColour) {
    vec3<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<F32> ToFloatColour(const vec4<U8>& byteColour) {
    vec4<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U8>& byteColour) {
    vec3<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec4<F32> ToFloatColour(const vec4<U32>& uintColour) {
    vec4<F32> tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U32>& uintColour) {
    vec3<F32> tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
}

F32 PACK_VEC3(const vec3<F32>& value) {
    return PACK_VEC3(value.x, value.y, value.z);
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

void RecordFloatEvent(const char* eventName, F32 eventValue, U64 timestamp) {
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

void PlotFloatEvents(const char* eventName,
                     vectorImpl<GlobalFloatEvent> eventsCopy,
                     GraphPlot2D& targetGraph) {
    targetGraph._plotName = eventName;
    targetGraph._coords.clear();
    for (GlobalFloatEvent& crtEvent : eventsCopy) {
        if (std::strcmp(eventName, crtEvent._eventName.c_str()) == 0) {
            vectorAlg::emplace_back(
                targetGraph._coords,
                Time::MicrosecondsToMilliseconds<F32>(crtEvent._timeStamp),
                crtEvent._eventValue);
        }
    }
}

};  // namespace Util
};  // namespace Divide

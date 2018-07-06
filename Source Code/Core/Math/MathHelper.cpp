#include "stdafx.h"

#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

namespace Divide {
namespace Util {

static boost::thread_specific_ptr<vectorFast<GlobalFloatEvent>> _globalFloatEvents;

void ToByteColour(const FColour& floatColour, UColour& colourOut) {
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

void ToIntColour(const FColour& floatColour, vec4<I32>& colourOut) {
    colourOut.set(FLOAT_TO_SCHAR_SNORM(floatColour.r),
                  FLOAT_TO_SCHAR_SNORM(floatColour.g),
                  FLOAT_TO_SCHAR_SNORM(floatColour.b),
                  FLOAT_TO_SCHAR_SNORM(floatColour.a));
}

void ToIntColour(const vec3<F32>& floatColour, vec3<I32>& colourOut) {
    colourOut.set(to_U32(FLOAT_TO_SCHAR_SNORM(floatColour.r)),
                  to_U32(FLOAT_TO_SCHAR_SNORM(floatColour.g)),
                  to_U32(FLOAT_TO_SCHAR_SNORM(floatColour.b)));
}

void ToUIntColour(const FColour& floatColour, vec4<U32>& colourOut) {
    colourOut.set(FLOAT_TO_CHAR_SNORM(floatColour.r),
                  FLOAT_TO_CHAR_SNORM(floatColour.g),
                  FLOAT_TO_CHAR_SNORM(floatColour.b),
                  FLOAT_TO_CHAR_SNORM(floatColour.a));
}

void ToUIntColour(const vec3<F32>& floatColour, vec3<U32>& colourOut) {
    colourOut.set(to_U32(FLOAT_TO_CHAR_SNORM(floatColour.r)),
                  to_U32(FLOAT_TO_CHAR_SNORM(floatColour.g)),
                  to_U32(FLOAT_TO_CHAR_SNORM(floatColour.b)));
}

void ToFloatColour(const UColour& byteColour, FColour& colourOut) {
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

void ToFloatColour(const vec4<U32>& uintColour, FColour& colourOut) {
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

UColour ToByteColour(const FColour& floatColour) {
    UColour tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec3<U8> ToByteColour(const vec3<F32>& floatColour) {
    vec3<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec4<I32> ToIntColour(const FColour& floatColour) {
    vec4<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<I32> ToIntColour(const vec3<F32>& floatColour) {
    vec3<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<U32> ToUIntColour(const FColour& floatColour) {
    vec4<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<U32> ToUIntColour(const vec3<F32>& floatColour) {
    vec3<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

FColour ToFloatColour(const UColour& byteColour) {
    FColour tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U8>& byteColour) {
    vec3<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

FColour ToFloatColour(const vec4<U32>& uintColour) {
    FColour tempColour;
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

void UNPACK_VEC3(const F32 src, vec3<F32>& res) {
    UNPACK_FLOAT(src, res.x, res.y, res.z);
}

vec3<F32> UNPACK_VEC3(const F32 src) {
    vec3<F32> res;
    UNPACK_VEC3(src, res);
    return res;
}

U32 PACK_11_11_10(const vec3<F32>& value) {
    return Divide::PACK_11_11_10(value.x, value.y, value.z);
}

void UNPACK_11_11_10(const U32 src, vec3<F32>& res) {
    Divide::UNPACK_11_11_10(src, res.x, res.y, res.z);
}

void Normalize(vec3<F32>& inputRotation, bool degrees, bool normYaw, bool normPitch, bool normRoll) noexcept {
    if (normYaw) {
        F32 yaw = degrees ? Angle::to_RADIANS(inputRotation.yaw)
                          : inputRotation.yaw;
        if (yaw < -M_PI) {
            yaw = fmod(yaw, to_F32(M_PI) * 2.0f);
            if (yaw < -M_PI) {
                yaw += to_F32(M_PI) * 2.0f;
            }
            inputRotation.yaw = Angle::to_DEGREES(yaw);
        } else if (yaw > M_PI) {
            yaw = fmod(yaw, to_F32(M_PI) * 2.0f);
            if (yaw > M_PI) {
                yaw -= to_F32(M_PI) * 2.0f;
            }
            inputRotation.yaw = degrees ? Angle::to_DEGREES(yaw) : yaw;
        }
    }
    if (normPitch) {
        F32 pitch = degrees ? Angle::to_RADIANS(inputRotation.pitch)
                            : inputRotation.pitch;
        if (pitch < -M_PI) {
            pitch = fmod(pitch, to_F32(M_PI) * 2.0f);
            if (pitch < -M_PI) {
                pitch += to_F32(M_PI) * 2.0f;
            }
            inputRotation.pitch = Angle::to_DEGREES(pitch);
        } else if (pitch > M_PI) {
            pitch = fmod(pitch, to_F32(M_PI) * 2.0f);
            if (pitch > M_PI) {
                pitch -= to_F32(M_PI) * 2.0f;
            }
            inputRotation.pitch =
                degrees ? Angle::to_DEGREES(pitch) : pitch;
        }
    }
    if (normRoll) {
        F32 roll = degrees ? Angle::to_RADIANS(inputRotation.roll)
                           : inputRotation.roll;
        if (roll < -M_PI) {
            roll = fmod(roll, to_F32(M_PI) * 2.0f);
            if (roll < -M_PI) {
                roll += to_F32(M_PI) * 2.0f;
            }
            inputRotation.roll = Angle::to_DEGREES(roll);
        } else if (roll > M_PI) {
            roll = fmod(roll, to_F32(M_PI) * 2.0f);
            if (roll > M_PI) {
                roll -= to_F32(M_PI) * 2.0f;
            }
            inputRotation.roll = degrees ? Angle::to_DEGREES(roll) : roll;
        }
    }
}

void FlushFloatEvents() {
    vectorFast<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorFast<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vec->clear();
}

void RecordFloatEvent(const char* eventName, F32 eventValue, U64 timestamp) {
    vectorFast<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if( !vec ) {
        vec = new vectorFast<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }
    vectorAlg::emplace_back(*vec, eventName, eventValue, timestamp);
}

const vectorFast<GlobalFloatEvent>& GetFloatEvents() {
    vectorFast<GlobalFloatEvent>* vec = _globalFloatEvents.get();
    if (!vec) {
        vec = new vectorFast<GlobalFloatEvent>();
        _globalFloatEvents.reset(vec);
    }

    return *vec;
}

void PlotFloatEvents(const char* eventName,
                     vectorFast<GlobalFloatEvent> eventsCopy,
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

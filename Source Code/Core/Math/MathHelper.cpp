#include "stdafx.h"

#include "Headers/MathMatrices.h"
#include "Headers/Quaternion.h"

#include <glm/glm.hpp>

namespace Divide {
namespace Util {

bool IntersectCircles(const Circle& cA, const Circle& cB, vec2<F32>* pointsOut) noexcept {
    assert(pointsOut != nullptr);

    const F32 x0 = cA.center[0];
    const F32 y0 = cA.center[1];
    const F32 r0 = cA.radius;

    const F32 x1 = cB.center[0];
    const F32 y1 = cB.center[1];
    const F32 r1 = cB.radius;
    
    /* dx and dy are the vertical and horizontal distances between
     * the circle centers.
     */
    const F32 dx = x1 - x0;
    const F32 dy = y1 - y0;

    /* Determine the straight-line distance between the centers. */
    //d = sqrt((dy*dy) + (dx*dx));
    const F32 d = hypot(dx, dy); // Suggested by Keith Briggs

    /* Check for solvability. */
    if (d > (r0 + r1))
    {
        /* no solution. circles do not intersect. */
        return false;
    }
    if (d < fabs(r0 - r1))
    {
        /* no solution. one circle is contained in the other */
        return false;
    }

    /* 'point 2' is the point where the line through the circle
     * intersection points crosses the line between the circle
     * centers.
     */

     /* Determine the distance from point 0 to point 2. */
    const F32 a = ((r0*r0) - (r1*r1) + (d*d)) / (2.0f * d);

    /* Determine the coordinates of point 2. */
    const F32 x2 = x0 + (dx * a / d);
    const F32 y2 = y0 + (dy * a / d);

    /* Determine the distance from point 2 to either of the
     * intersection points.
     */
    const F32 h = sqrt((r0*r0) - (a*a));

    /* Now determine the offsets of the intersection points from
     * point 2.
     */
    const F32 rx = -dy * (h / d);
    const F32 ry = dx * (h / d);

    /* Determine the absolute intersection points. */
    
    pointsOut[0].x = x2 + rx;
    pointsOut[1].x = x2 - rx;

    pointsOut[0].y = y2 + ry;
    pointsOut[1].y = y2 - ry;

    return true;
}

void ToByteColour(const FColour& floatColour, UColour& colourOut) noexcept {
    colourOut.set(FLOAT_TO_CHAR(floatColour.r),
                  FLOAT_TO_CHAR(floatColour.g),
                  FLOAT_TO_CHAR(floatColour.b),
                  FLOAT_TO_CHAR(floatColour.a));
}

void ToByteColour(const vec3<F32>& floatColour, vec3<U8>& colourOut) noexcept {
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

void ToIntColour(const vec3<F32>& floatColour, vec3<I32>& colourOut) noexcept {
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

void ToUIntColour(const vec3<F32>& floatColour, vec3<U32>& colourOut) noexcept {
    colourOut.set(to_U32(FLOAT_TO_CHAR_SNORM(floatColour.r)),
                  to_U32(FLOAT_TO_CHAR_SNORM(floatColour.g)),
                  to_U32(FLOAT_TO_CHAR_SNORM(floatColour.b)));
}

void ToFloatColour(const UColour& byteColour, FColour& colourOut) noexcept {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                  CHAR_TO_FLOAT_SNORM(byteColour.g),
                  CHAR_TO_FLOAT_SNORM(byteColour.b),
                  CHAR_TO_FLOAT_SNORM(byteColour.a));
}

void ToFloatColour(const vec3<U8>& byteColour, vec3<F32>& colourOut) noexcept {
    colourOut.set(CHAR_TO_FLOAT_SNORM(byteColour.r),
                  CHAR_TO_FLOAT_SNORM(byteColour.g),
                  CHAR_TO_FLOAT_SNORM(byteColour.b));
}

void ToFloatColour(const vec4<U32>& uintColour, FColour& colourOut) noexcept {
    colourOut.set(uintColour.r / 255.0f,
                  uintColour.g / 255.0f,
                  uintColour.b / 255.0f,
                  uintColour.a / 255.0f);
}

void ToFloatColour(const vec3<U32>& uintColour, vec3<F32>& colourOut) noexcept {
    colourOut.set(uintColour.r / 255.0f,
                  uintColour.g / 255.0f,
                  uintColour.b / 255.0f);
}

UColour ToByteColour(const FColour& floatColour) noexcept {
    UColour tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec3<U8> ToByteColour(const vec3<F32>& floatColour) noexcept {
    vec3<U8> tempColour;
    ToByteColour(floatColour, tempColour);
    return tempColour;
}

vec4<I32> ToIntColour(const FColour& floatColour) {
    vec4<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<I32> ToIntColour(const vec3<F32>& floatColour) noexcept {
    vec3<I32> tempColour;
    ToIntColour(floatColour, tempColour);
    return tempColour;
}

vec4<U32> ToUIntColour(const FColour& floatColour) {
    vec4<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

vec3<U32> ToUIntColour(const vec3<F32>& floatColour) noexcept {
    vec3<U32> tempColour;
    ToUIntColour(floatColour, tempColour);
    return tempColour;
}

FColour ToFloatColour(const UColour& byteColour) {
    FColour tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U8>& byteColour) noexcept {
    vec3<F32> tempColour;
    ToFloatColour(byteColour, tempColour);
    return tempColour;
}

FColour ToFloatColour(const vec4<U32>& uintColour) {
    FColour tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
}

vec3<F32> ToFloatColour(const vec3<U32>& uintColour) noexcept {
    vec3<F32> tempColour;
    ToFloatColour(uintColour, tempColour);
    return tempColour;
}

F32 PACK_VEC3(const vec3<F32>& value) {
    return PACK_VEC3(value.x, value.y, value.z);
}

U32 PACK_VEC2(const vec2<F32>& value) {
    return PACK_VEC2(value.x, value.y);
}

void UNPACK_VEC3(const F32 src, vec3<F32>& res) {
    UNPACK_FLOAT(src, res.x, res.y, res.z);
}

vec3<F32> UNPACK_VEC3(const F32 src) {
    vec3<F32> res;
    UNPACK_VEC3(src, res);
    return res;
}

void UNPACK_VEC2(const U32 src, vec2<F32>& res) {
    UNPACK_VEC2(src, res.x, res.y);
}

void UNPACK_VEC2(const U32 src, F32& x, F32& y) noexcept {
    x = (src >> 16) / 65535.0f;
    y = (src & 0xFFFF) / 65535.0f;
}

U32 PACK_HALF2x16(const vec2<F32>& value) {
    return to_U32(glm::packHalf2x16(glm::mediump_vec2(value.x, value.y)));
}

void UNPACK_HALF2x16(const U32 src, vec2<F32>& value) {
    const glm::vec2 ret = glm::unpackHalf2x16(src);
    value.set(ret.x, ret.y);
}

U32 PACK_UNORM4x8(const vec4<U8>& value) {
    return to_U32(glm::packUnorm4x8(glm::mediump_vec4(value.x, value.y, value.z, value.w)));
}

void UNPACK_UNORM4x8(const U32 src, vec4<U8>& value) {
    const glm::vec4 ret = glm::unpackUnorm4x8(src);
    value.set(ret.x, ret.y, ret.z, ret.w);
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
};  // namespace Util
};  // namespace Divide

/*
   Copyright (c) 2018 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _QUATERNION_H_
#define _QUATERNION_H_

/*
http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
Quaternion class based on code from " OpenGL:Tutorials:Using Quaternions to
represent rotation "
*/
#include "Core/Math/Headers/MathMatrices.h"

namespace Divide {

template <typename T>
class Quaternion {
    static_assert(std::is_arithmetic<T>::value &&
                  !std::is_same<T, bool>::value,
                  "non-arithmetic quaternion type");
   public:
    Quaternion() noexcept;
    Quaternion(T x, T y, T z, T w) noexcept;
    Quaternion(const vec4<T>& values) noexcept;

    template <typename U = T>
    Quaternion(__m128 reg, typename std::enable_if<std::is_same<U, F32>::value>::type* = nullptr) noexcept;

    Quaternion(const mat3<T>& rotationMatrix) noexcept;
    Quaternion(const vec3<T>& axis, Angle::DEGREES<T> angle) noexcept;
    Quaternion(Angle::DEGREES<T> pitch, Angle::DEGREES<T> yaw, Angle::DEGREES<T> roll) noexcept;
    Quaternion(const Quaternion& q) noexcept;

    Quaternion& operator=(const Quaternion& q);

    inline T dot(const Quaternion& rq) const;
    inline T magnitude() const;
    inline T magnituteSq() const;

    inline bool compare(const Quaternion& rq, Angle::DEGREES<T> tolerance = 1e-3f) const;

    inline void set(const vec4<T>& values);
    inline void set(T x, T y, T z, T w);
    inline void set(const Quaternion& q);

    //! normalizing a quaternion works similar to a vector. This method will not
    //do anything
    //! if the quaternion is close enough to being unit-length.
    inline void normalize();
    inline Quaternion inverse() const;

    //! We need to get the inverse of a quaternion to properly apply a
    //quaternion-rotation to a vector
    //! The conjugate of a quaternion is the same as the inverse, as long as the
    //quaternion is unit-length
    inline Quaternion getConjugate() const;

    //! Multiplying q1 with q2 applies the rotation q2 to q1
    //! the constructor takes its arguments as (x, y, z, w)
    inline Quaternion operator*(const Quaternion& rq) const;

    //! Multiply so that rotations are applied in a left to right order.
    inline Quaternion& operator*=(const Quaternion& rq);


    //! Dividing q1 by q2
    inline Quaternion operator/(const Quaternion& rq) const;
    inline Quaternion& operator/=(const Quaternion& rq);

    //! Multiplying a quaternion q with a vector v applies the q-rotation to v
    vec3<T> operator*(const vec3<T>& vec) const;

    bool operator==(const Quaternion& rq) const;
    bool operator!=(const Quaternion& rq) const;

    inline Quaternion& operator+=(const Quaternion& rq);

    inline Quaternion& operator-=(const Quaternion& rq);

    inline Quaternion& operator*=(T scalar);

    inline Quaternion& operator/=(T scalar);

    inline Quaternion operator+(const Quaternion& rq) const;

    inline Quaternion operator-(const Quaternion& rq) const;

    inline Quaternion operator*(T scalar) const;

    inline Quaternion operator/(T scalar) const;

    inline void slerp(const Quaternion& q, F32 t);

    void slerp(const Quaternion& q0, const Quaternion& q1, F32 t);

    //! Convert from Axis Angle
    void fromAxisAngle(const vec3<T>& v, Angle::DEGREES<T> angle);

    inline void fromEuler(const vec3<Angle::DEGREES<T>>& v);

    //! Convert from Euler Angles
    void fromEuler(Angle::DEGREES<T> pitch, Angle::DEGREES<T> yaw, Angle::DEGREES<T> roll);

    void fromLookAt(const vec3<F32>& fwdDirection, const vec3<F32>& upDirection);

    // a la Ogre3D
    void fromMatrix(const mat3<T>& rotationMatrix);

    //! Convert to Matrix
    void getMatrix(mat3<F32>& outMatrix) const;

    //! Convert to Axis/Angles
    void getAxisAngle(vec3<T>* axis, Angle::DEGREES<T>* angle) const;

    void getEuler(vec3<Angle::RADIANS<T>>& euler) const;


    /// X/Y/Z Axis get/set a la Ogre: OgreQuaternion.cpp
    void fromAxes(const vec3<T>* axis);
    void fromAxes(const vec3<T>& xAxis, const vec3<T>& yAxis, const vec3<T>& zAxis);
    void toAxes(vec3<T>* axis) const;
    void toAxes(vec3<T>& xAxis, vec3<T>& yAxis, vec3<T>& zAxis) const;
    vec3<T> xAxis() const;
    vec3<T> yAxis() const;
    vec3<T> zAxis() const;

    inline F32 X() const noexcept;
    inline F32 Y() const noexcept;
    inline F32 Z() const noexcept;
    inline F32 W() const noexcept;

    inline void X(F32 x) noexcept;
    inline void Y(F32 y) noexcept;
    inline void Z(F32 z) noexcept;
    inline void W(F32 w) noexcept;

    inline void identity();

    inline const vec4<T>& asVec4() const;

   private:
    vec4<T> _elements;
};

/// get the shortest arc quaternion to rotate vector 'v' to the target vector 'u'
/// (from Ogre3D!)
template <typename T>
inline Quaternion<T> RotationFromVToU(const vec3<T>& v, const vec3<T>& u,
                                      const vec3<T>& fallbackAxis = VECTOR3_ZERO);
template <typename T>
inline Quaternion<T> Slerp(const Quaternion<T>& q0, const Quaternion<T>& q1, F32 t);

template <typename T>
inline mat3<T> GetMatrix(const Quaternion<T>& q);

template <typename T>
inline vec3<Angle::RADIANS<T>> GetEuler(const Quaternion<T>& q);

};  // namespace Divide

#endif

#include "Quaternion.inl"
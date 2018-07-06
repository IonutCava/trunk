/*
   Copyright (c) 2016 DIVIDE-Studio
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
    Quaternion();
    Quaternion(T x, T y, T z, T w);
    Quaternion(const vec4<T>& values);
    Quaternion(const mat3<T>& rotationMatrix);
    Quaternion(const vec3<T>& axis, T angle, bool inDegrees = true);
    Quaternion(T pitch, T yaw, T roll, bool inDegrees = true);
    Quaternion(const Quaternion& q);

    inline T dot(const Quaternion& rq) const;
    inline T magnitude() const;
    inline T magnituteSq() const;

    inline bool compare(const Quaternion& rq, F32 tolerance = 1e-3f) const;

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
    void fromAxisAngle(const vec3<T>& v, T angle, bool inDegrees = true);

    inline void fromEuler(const vec3<T>& v, bool inDegrees = true);

    //! Convert from Euler Angles
    void fromEuler(T pitch, T yaw, T roll, bool inDegrees = true);

    // a la Ogre3D
    void fromMatrix(const mat3<T>& rotationMatrix);

    //! Convert to Matrix
    void getMatrix(mat4<F32>& outMatrix) const;

    //! Convert to Axis/Angles
    void getAxisAngle(vec3<T>* axis, T* angle, bool inDegrees) const;

    void getEuler(vec3<T>& euler, bool toDegrees = false) const;

    inline F32 X() const;
    inline F32 Y() const;
    inline F32 Z() const;
    inline F32 W() const;

    inline void X(F32 x);
    inline void Y(F32 y);
    inline void Z(F32 z);
    inline void W(F32 w);

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
inline mat4<T> GetMatrix(const Quaternion<T>& q);

template <typename T>
inline vec3<T> GetEuler(const Quaternion<T>& q, const bool toDegrees = false);

};  // namespace Divide

#endif

#include "Quaternion.inl"
/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _QUATERNION_INL_
#define _QUATERNION_INL_

namespace Divide {
template <typename T>
Quaternion<T>::Quaternion()
{
    identity();
}

template <typename T>
Quaternion<T>::Quaternion(T x, T y, T z, T w)
{
    _elements.set(x, y, z, w);
}

template <typename T>
Quaternion<T>::Quaternion(const vec4<T>& values)
{
    _elements.set(values);
}

template <typename T>
Quaternion<T>::Quaternion(const mat3<T>& rotationMatrix)
{
    fromMatrix(rotationMatrix);
}

template <typename T>
Quaternion<T>::Quaternion(const vec3<T>& axis, T angle, bool inDegrees = true)
{
    fromAxisAngle(axis, angle, inDegrees);
}

template <typename T>
Quaternion<T>::Quaternion(T pitch, T yaw, T roll, bool inDegrees = true)
{
    fromEuler(pitch, yaw, roll, inDegrees);
}

template <typename T>
Quaternion<T>::Quaternion(const Quaternion& q)
{
    set(q);
}

template <typename T>
inline T Quaternion<T>::dot(const Quaternion<T>& rq) const {
    return _elements.dot(rq._elements);
}

template <typename T>
inline T Quaternion<T>::magnitude() const {
    return _elements.length();
}

template <typename T>
inline T Quaternion<T>::magnituteSq() const {
    return _elements.lengthSquared();
}

template <typename T>
inline bool Quaternion<T>::compare(const Quaternion<T>& rq,
                                   F32 tolerance = 1e-3f) const {
    T angleRad = Angle::DegreesToRadians((T)std::acos(to_double(dot(rq))));
    F32 toleranceRad = Angle::DegreesToRadians(tolerance);

    return (std::abs(angleRad) <= toleranceRad) ||
           FLOAT_COMPARE_TOLERANCE(angleRad, to_float(M_PI),
                                   toleranceRad);
}

template <typename T>
inline void Quaternion<T>::set(const vec4<T>& values) {
    _elements.set(values);
}

template <typename T>
inline void Quaternion<T>::set(T x, T y, T z, T w) {
    _elements.set(x, y, z, w);
}

template <typename T>
inline void Quaternion<T>::set(const Quaternion<T>& q) {
    set(q._elements);
}

template <typename T>
inline void Quaternion<T>::normalize() {
    _elements.normalize();
}

template <typename T>
inline Quaternion<T> Quaternion<T>::inverse() const {
    return getConjugate() * (1.0f / magnitude());
}

template <typename T>
inline Quaternion<T> Quaternion<T>::getConjugate() const {
    return Quaternion<T>(-X(), -Y(), -Z(), W());
}

template <typename T>
inline Quaternion<T> Quaternion<T>::operator*(const Quaternion<T>& rq) const {
    return Quaternion<T>(
        W() * rq.X() + X() * rq.W() + Y() * rq.Z() - Z() * rq.Y(),
        W() * rq.Y() + Y() * rq.W() + Z() * rq.X() - X() * rq.Z(),
        W() * rq.Z() + Z() * rq.W() + X() * rq.Y() - Y() * rq.X(),
        W() * rq.W() - X() * rq.X() - Y() * rq.Y() - Z() * rq.Z());
}

template <typename T>
inline Quaternion<T>& Quaternion<T>::operator*=(const Quaternion<T>& rq) {
    (*this) = rq * (*this);
    return (*this);
}

template <typename T>
vec3<T> Quaternion<T>::operator*(const vec3<T>& vec) const {
    // nVidia SDK implementation
    vec3<T> uv(Cross(_elements.xyz(), vec));
    return vec + (uv * (2.0f * W())) + (Cross(_elements.xyz(), uv) * 2.0f);
}

template <typename T>
bool Quaternion<T>::operator==(const Quaternion<T>& rq) const {
    return compare(rq);
}

template <typename T>
bool Quaternion<T>::operator!=(const Quaternion<T>& rq) const {
    return !(*this == rq);
}

template <typename T>
inline Quaternion<T>& Quaternion<T>::operator+=(const Quaternion<T>& rq) {
    _elements += rq._elements;
    return *this;
}

template <typename T>
inline Quaternion<T>& Quaternion<T>::operator-=(const Quaternion<T>& rq) {
    _elements -= rq._elements;
    return *this;
}

template <typename T>
inline Quaternion<T>& Quaternion<T>::operator*=(T scalar) {
    _elements *= scalar;
    return *this;
}

template <typename T>
inline Quaternion<T>& Quaternion<T>::operator/=(T scalar) {
    _elements /= scalar;
    return *this;
}

template <typename T>
inline Quaternion<T> Quaternion<T>::operator+(const Quaternion<T>& rq) const {
    Quaternion<T> tmp(*this);
    tmp += rq;
    return tmp;
}

template <typename T>
inline Quaternion<T> Quaternion<T>::operator-(const Quaternion<T>& rq) const {
    Quaternion<T> tmp(*this);
    tmp -= rq;
    return tmp;
}

template <typename T>
inline Quaternion<T> Quaternion<T>::operator*(T scalar) const {
    Quaternion<T> tmp(*this);
    tmp *= scalar;
    return tmp;
}

template <typename T>
inline Quaternion<T> Quaternion<T>::operator/(T scalar) const {
    Quaternion<T> tmp(*this);
    tmp /= scalar;
    return tmp;
}

template <typename T>
inline void Quaternion<T>::slerp(const Quaternion<T>& q, F32 t) {
    slerp(*this, q, t);
}

template <typename T>
void Quaternion<T>::slerp(const Quaternion<T>& q0, const Quaternion<T>& q1, F32 t) {
    F32 k0, k1;
    T cosomega = q0.dot(q1);
    Quaternion<T> q;
    if (cosomega < 0.0) {
        cosomega = -cosomega;
        q._elements.set(-q1._elements);
    } else {
        q._elements.set(q1._elements);
    }

    if (1.0 - cosomega > 1e-6) {
        F32 omega = to_float(std::acos(cosomega));
        F32 sinomega = to_float(std::sin(omega));
        k0 = to_float(std::sin((1.0f - t) * omega) / sinomega);
        k1 = to_float(std::sin(t * omega) / sinomega);
    } else {
        k0 = 1.0f - t;
        k1 = t;
    }
    _elements.set(q0._elements * k0 + q._elements * k1);
}

template <typename T>
void Quaternion<T>::fromAxisAngle(const vec3<T>& v, T angle,
                                  bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }
    angle *= 0.5f;
    vec3<T> vn(v);
    vn.normalize();

    _elements.set(vn * std::sin(angle), std::cos(angle));
}

template <typename T>
inline void Quaternion<T>::fromEuler(const vec3<T>& v, bool inDegrees = true) {
    fromEuler(v.pitch, v.yaw, v.roll, inDegrees);
}

template <typename T>
void Quaternion<T>::fromEuler(T pitch, T yaw, T roll, bool inDegrees = true) {
    T attitude = pitch;
    T heading = yaw;
    T bank = roll;

    if (inDegrees) {
        attitude = Angle::DegreesToRadians(attitude);
        heading = Angle::DegreesToRadians(heading);
        bank = Angle::DegreesToRadians(bank);
    }

    D32 c1 = std::cos(heading * 0.5);
    D32 s1 = std::sin(heading * 0.5);
    D32 c2 = std::cos(attitude * 0.5);
    D32 s2 = std::sin(attitude * 0.5);
    D32 c3 = std::cos(bank * 0.5);
    D32 s3 = std::sin(bank * 0.5);

    D32 c1c2 = c1 * c2;
    D32 s1s2 = s1 * s2;

    W(static_cast<T>(c1c2 * c3 - s1s2 * s3));
    X(static_cast<T>(c1c2 * s3 + s1s2 * c3));
    Y(static_cast<T>(s1 * c2 * c3 + c1 * s2 * s3));
    Z(static_cast<T>(c1 * s2 * c3 - s1 * c2 * s3));

    // normalize(); this method does produce a normalized quaternion
}

template <typename T>
void Quaternion<T>::fromMatrix(const mat3<T>& rotationMatrix) {
    // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
    // article "Quaternion Calculus and Fast Animation".

    T fTrace = rotationMatrix.m[0][0] + rotationMatrix.m[1][1] +
               rotationMatrix.m[2][2];
    T fRoot;

    if (fTrace > 0.0) {
        // |w| > 1/2, may as well choose w > 1/2
        fRoot = (T)std::sqrtf(to_float(fTrace) + 1.0f);  // 2w
        W(0.5f * fRoot);
        fRoot = 0.5f / fRoot;  // 1/(4w)
        X((rotationMatrix.m[2][1] - rotationMatrix.m[1][2]) * fRoot);
        Y((rotationMatrix.m[0][2] - rotationMatrix.m[2][0]) * fRoot);
        Z((rotationMatrix.m[1][0] - rotationMatrix.m[0][1]) * fRoot);
    } else {
        // |w| <= 1/2
        static size_t s_iNext[3] = {1, 2, 0};
        size_t i = 0;
        if (rotationMatrix.m[1][1] > rotationMatrix.m[0][0]) {
            i = 1;
        }
        if (rotationMatrix.m[2][2] > rotationMatrix.m[i][i]) {
            i = 2;
        }
        size_t j = s_iNext[i];
        size_t k = s_iNext[j];

        fRoot = static_cast<T>(std::sqrtf(
                    to_float(rotationMatrix.m[i][i] - rotationMatrix.m[j][j] -
                             rotationMatrix.m[k][k] + 1.0f)));
        T* apkQuat[3] = {&_elements.x, &_elements.y, &_elements.z};
        *apkQuat[i] = 0.5f * fRoot;
        fRoot = 0.5f / fRoot;
        W((rotationMatrix.m[k][j] - rotationMatrix.m[j][k]) * fRoot);
        *apkQuat[j] = (rotationMatrix.m[j][i] + rotationMatrix.m[i][j]) * fRoot;
        *apkQuat[k] = (rotationMatrix.m[k][i] + rotationMatrix.m[i][k]) * fRoot;
    }
}

template <typename T>
void Quaternion<T>::getMatrix(mat4<F32>& outMatrix) const {
    const T& x = X();
    const T& y = Y();
    const T& z = Z();
    const T& w = W();

    T xx = x * x;
    T xy = x * y;
    T xz = x * z;
    T xw = x * w;
    T yy = y * y;
    T yz = y * z;
    T yw = y * w;
    T zz = z * z;
    T zw = z * w;
    outMatrix.mat[0] = 1 - 2 * (yy + zz);
    outMatrix.mat[1] = 2 * (xy - zw);
    outMatrix.mat[2] = 2 * (xz + yw);
    outMatrix.mat[4] = 2 * (xy + zw);
    outMatrix.mat[5] = 1 - 2 * (xx + zz);
    outMatrix.mat[6] = 2 * (yz - xw);
    outMatrix.mat[8] = 2 * (xz - yw);
    outMatrix.mat[9] = 2 * (yz + xw);
    outMatrix.mat[10] = 1 - 2 * (xx + yy);
}

template <typename T>
void Quaternion<T>::getAxisAngle(vec3<T>* axis, T* angle, bool inDegrees) const {
    axis->set(_elements / _elements.xyz().length());
    *angle = inDegrees ? Angle::RadiansToDegrees(std::acos(W()) * 2.0f)
                       : std::acos(W()) * 2.0f;
}

template <typename T>
void Quaternion<T>::getEuler(vec3<T>* euler, bool toDegrees = false) const {
    T heading = 0, attitude = 0, bank = 0;
    const T& x = X();
    const T& y = Y();
    const T& z = Z();
    const T& w = W();
    T sqx = x * x;
    T sqy = y * y;
    T sqz = z * z;
    T sqw = w * w;
    T test = x * y + z * w;
    T unit = sqx + sqy + sqz +
             sqw;  // if normalized is one, otherwise is correction factor

    if (test > (0.5f - EPSILON_F32) * unit) {  // singularity at north pole
        heading = 2 * std::atan2(x, w);
        attitude = static_cast<T>(M_PI2);
        bank = 0;
    } else if (test <
               -(0.5f - EPSILON_F32) * unit) {  // singularity at south pole
        heading = -2 * std::atan2(x, w);
        attitude = -static_cast<T>(M_PI2);
        bank = 0;
    } else {
        T x2 = 2 * x;
        T y2 = 2 * y;

        heading = std::atan2(y2 * w - x2 * z, sqx - sqy - sqz + sqw);
        attitude = std::asin(2 * test / unit);
        bank = std::atan2(x2 * w - y2 * z, -sqx + sqy - sqz + sqw);
    }
    // Convert back from Z = pitch to Z = roll
    if (toDegrees) {
        euler->yaw = Angle::RadiansToDegrees(heading);
        euler->pitch = Angle::RadiansToDegrees(bank);
        euler->roll = Angle::RadiansToDegrees(attitude);
    } else {
        euler->yaw = heading;
        euler->pitch = bank;
        euler->roll = attitude;
    }
}

template <typename T>
inline F32 Quaternion<T>::X() const {
    return _elements.x;
}

template <typename T>
inline F32 Quaternion<T>::Y() const {
    return _elements.y;
}

template <typename T>
inline F32 Quaternion<T>::Z() const {
    return _elements.z;
}

template <typename T>
inline F32 Quaternion<T>::W() const {
    return _elements.w;
}

template <typename T>
inline void Quaternion<T>::X(F32 x) {
    _elements.x = x;
}

template <typename T>
inline void Quaternion<T>::Y(F32 y) {
    _elements.y = y;
}

template <typename T>
inline void Quaternion<T>::Z(F32 z) {
    _elements.z = z;
}

template <typename T>
inline void Quaternion<T>::W(F32 w) {
    _elements.w = w;
}

template <typename T>
inline void Quaternion<T>::identity() {
    _elements.set(0, 0, 0, 1);
}

template <typename T>
inline const vec4<T>& Quaternion<T>::asVec4() const {
    return _elements;
}


/// get the shortest arc quaternion to rotate vector 'v' to the target vector
/// 'u'(from Ogre3D!)
template <typename T>
inline Quaternion<T> RotationFromVToU(
    const vec3<T>& v, const vec3<T>& u,
    const vec3<T> fallbackAxis) {
    // Based on Stan Melax's article in Game Programming Gems
    Quaternion<T> q;
    // Copy, since cannot modify local
    vec3<T> v0 = v;
    vec3<T> v1 = u;
    v0.normalize();
    v1.normalize();

    T d = v0.dot(v1);
    // If dot == 1, vectors are the same
    if (d >= 1.0f) {
        return q;
    } else if (d < (1e-6f - 1.0f)) {
        if (!fallbackAxis.compare(VECTOR3_ZERO)) {
            // rotate 180 degrees about the fallback axis
            q.fromAxisAngle(fallbackAxis, Angle::DegreesToRadians(to_float(M_PI)));
        } else {
            // Generate an axis
            vec3<T> axis;
            axis.cross(WORLD_X_AXIS, v);

            if (axis.isZeroLength())  // pick another if collinear
                axis.cross(WORLD_Y_AXIS, v);

            axis.normalize();
            q.fromAxisAngle(axis, Angle::DegreesToRadians(to_float(M_PI)));
        }
    } else {
        F32 s = std::sqrtf((1 + d) * 2);
        F32 invs = 1 / s;

        vec3<T> c(Cross(v0, v1) * invs);
        q.set(c.x, c.y, c.z, s * 0.5f);
        q.normalize();
    }

    return q;
}

template <typename T>
inline Quaternion<T> Slerp(const Quaternion<T>& q0, const Quaternion<T>& q1,
                           F32 t) {
    Quaternion<T> temp;
    temp.slerp(q0, q1, t);
    return temp;
}

template <typename T>
inline mat4<T> GetMatrix(const Quaternion<T>& q) {
    mat4<T> temp;
    q.getMatrix(temp);
    return temp;
}

template <typename T>
inline vec3<T> GetEuler(const Quaternion<T>& q, const bool toDegrees) {
    vec3<T> euler;
    q.getEuler(&euler, toDegrees);
    return euler;
}
};  // namespace Divide

#endif  //_QUATERNION_INL_
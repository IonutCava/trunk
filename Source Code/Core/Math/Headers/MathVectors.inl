/* Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _CORE_MATH_MATH_VECTORS_H_
#define _CORE_MATH_MATH_VECTORS_H_

namespace Divide {
/*
*  useful vector functions
*/
/// general vec2 cross function
template <typename T>
inline vec2<T> cross(const vec2<T> &v1, const vec2<T> &v2) {
    return v1.x * v2.y - v1.y * v2.x;
}

template <typename T>
inline vec2<T> inverse(const vec2<T> &v) {
    return vec2<T>(v.y, v.x);
}

/// multiply a vector by a value
template <typename T>
inline vec2<T> operator*(T fl, const vec2<T> &v) {
    return vec2<T>(v.x * fl, v.y * fl);
}

/// general vec2 dot product
template <typename T>
inline T dot(const vec2<T> &a, const vec2<T> &b) {
    return (a.x * b.x + a.y * b.y);
}

/// multiply a vector by a value
template <typename T>
inline vec3<T> operator*(T fl, const vec3<T> &v) {
    return vec3<T>(v.x * fl, v.y * fl, v.z * fl);
}

/// general vec3 dot product
template <typename T>
inline T dot(const vec3<T> &a, const vec3<T> &b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

/// general vec3 cross function
template <typename T>
inline vec3<T> cross(const vec3<T> &v1, const vec3<T> &v2) {
    return vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                   v1.x * v2.y - v1.y * v2.x);
}

template <typename T>
inline vec3<T> inverse(const vec3<T> &v) {
    return vec3<T>(v.z, v.y, v.x);
}
/// multiply a vector by a value
template <typename T>
inline vec4<T> operator*(T fl, const vec4<T> &v) {
    return vec4<T>(v.x * fl, v.y * fl, v.z * fl, v.w * fl);
}

/*
*  vec2 inline definitions
*/

/// convert the vector to unit length
template <typename T>
inline T vec2<T>::normalize() {
    T l = this->length();

    if (l < EPSILON) {
        return 0;
    }

    T inv = 1.0f / l;
    this->x *= inv;
    this->y *= inv;
    return l;
}

/// compare 2 vectors using the given tolerance
template <typename T>
inline bool vec2<T>::compare(const vec2 &_v, F32 epsi = EPSILON_F32) const {
    return (FLOAT_COMPARE_TOLERANCE(this->x, _v.x, epsi) &&
            FLOAT_COMPARE_TOLERANCE(this->y, _v.y, epsi));
}

/// return the projection factor from *this to the line determined by points vA
/// and vB
template <typename T>
inline T vec2<T>::projectionOnLine(const vec2 &vA, const vec2 &vB) const {
    vec2 v(vB - vA);
    return v.dot(*this - vA) / v.dot(v);
}

/// get the dot product between this vector and the specified one
template <typename T>
inline T vec2<T>::dot(const vec2 &v) const {
    return ((this->x * v.x) + (this->y * v.y));
}

/// round both values
template <typename T>
inline void vec2<T>::round() {
    set((T)std::roundf(this->x), (T)std::roundf(this->y));
}

/// export the vector's components in the first 2 positions of the specified
/// array
template <typename T>
inline void vec2<T>::get(T *v) const {
    v[0] = (T) this->_v[0];
    v[1] = (T) this->_v[1];
}

/// return the coordinates of the closest point from *this to the line
/// determined by points vA and vB
template <typename T>
inline vec2<T> closestPointOnLine(const vec2<T> &vA, const vec2<T> &vB) {
    return (((vB - vA) * this->projectionOnLine(vA, vB)) + vA);
}

/// return the coordinates of the closest point from *this to the segment
/// determined by points vA and vB
template <typename T>
inline vec2<T> closestPointOnSegment(const vec2<T> &vA, const vec2<T> &vB) {
    T factor = this->projectionOnLine(vA, vB);

    if (factor <= 0) return vA;

    if (factor >= 1) return vB;

    return (((vB - vA) * factor) + vA);
}
/// lerp between this and the specified vector by the specified amount
template <typename T>
inline void vec2<T>::lerp(const vec2 &v, T factor) {
    set((*this * (1 - factor)) + (v * factor));
}

/// lerp between this and the specified vector by the specified amount for each
/// component
template <typename T>
inline void vec2<T>::lerp(const vec2 &v, const vec2 &factor) {
    set((*this * (1 - factor)) + (v * factor));
}

/// linear interpolation between 2 vectors
template <typename T>
inline vec2<T> lerp(const vec2<T> &u, const vec2<T> &v, T factor) {
    return ((u * (1 - factor)) + (v * factor));
}

/// linear interpolation between 2 vectors based on separate x and y factors
template <typename T>
inline vec2<T> lerp(const vec2<T> &u, const vec2<T> &v, const vec2<T> &factor) {
    return (vec2((u.x * (1 - factor.x)) + (v.x * factor.x),
                 (u.y * (1 - factor.y)) + (v.y * factor.y)));
}

/*
*  vec3 inline definitions
*/
/// compare 2 vectors within the specified tolerance
template <typename T>
inline bool vec3<T>::compare(const vec3 &v, F32 epsi = EPSILON_F32) const {
    return FLOAT_COMPARE_TOLERANCE(this->x, v.x, epsi) &&
           FLOAT_COMPARE_TOLERANCE(this->y, v.y, epsi) &&
           FLOAT_COMPARE_TOLERANCE(this->z, v.z, epsi);
}

/// uniform vector: x = y = z
template <typename T>
inline bool vec3<T>::isUniform() const {
    return FLOAT_COMPARE(this->x, this->y) && FLOAT_COMPARE(this->y, this->z);
}

/// return the squared distance of the vector
template <typename T>
inline T vec3<T>::lengthSquared() const {
    return this->x * this->x + this->y * this->y + this->z * this->z;
}

/// transform the vector to unit length
template <typename T>
inline T vec3<T>::normalize() {
    T l = this->length();

    if (l < EPSILON_F32) return 0.0f;

    // multiply by the inverse length
    *this *= (1.0f / l);

    return l;
}

/// set this vector to be equal to the cross of the 2 specified vectors
template <typename T>
inline void vec3<T>::cross(const vec3 &v1, const vec3 &v2) {
    this->x = v1.y * v2.z - v1.z * v2.y;
    this->y = v1.z * v2.x - v1.x * v2.z;
    this->z = v1.x * v2.y - v1.y * v2.x;
}

/// set this vector to be equal to the cross between itself and the specified
/// vector
template <typename T>
inline void vec3<T>::cross(const vec3 &v2) {
    this->cross(*this, v2);
}

/// calculate the dot product between this vector and the specified one
template <typename T>
inline T vec3<T>::dot(const vec3 &v) const {
    return Divide::dot(*this, v);
}

/// compute the vector's distance to another specified vector
template <typename T>
inline T vec3<T>::distance(const vec3 &v) const {
    return std::sqrt(((v.x - this->x) * (v.x - this->x)) +
                     ((v.y - this->y) * (v.y - this->y)) +
                     ((v.z - this->z) * (v.z - this->z)));
}
/// compute the vector's squared distance to another specified vector
template <typename T>
inline T vec3<T>::distanceSquared(const vec3 &v) const {
    return (*this - v).lengthSquared();
}
/// returns the angle in radians between '*this' and 'v'
template <typename T>
inline T vec3<T>::angle(vec3 &v) const {
    T angle =
        (T)std::fabs(std::acos(this->dot(v) / (this->length() * v.length())));

    if (angle < EPSILON) {
        return 0;
    }
    return angle;
}

/// get the direction vector to the specified point
template <typename T>
inline vec3<T> vec3<T>::direction(const vec3 &u) const {
    vec3 vector(u.x - this->x, u.y - this->y, u.z - this->z);
    vector.normalize();
    return vector;
}

/// project this vector on the line defined by the 2 points(A, B)
template <typename T>
inline T vec3<T>::projectionOnLine(const vec3 &vA, const vec3 &vB) const {
    vec3 vector(vB - vA);
    return vector.dot(*this - vA) / vector.dot(vector);
}

/// lerp between this and the specified vector by the specified amount
template <typename T>
inline void vec3<T>::lerp(const vec3 &v, T factor) {
    set((*this * (1 - factor)) + (v * factor));
}

/// lerp between this and the specified vector by the specified amount for each
/// component
template <typename T>
inline void vec3<T>::lerp(const vec3 &v, const vec3 &factor) {
    set((*this * (1 - factor)) + (v * factor));
}

/// rotate this vector on the X axis
template <typename T>
inline void vec3<T>::rotateX(D32 radians) {
    this->y = static_cast<T>(std::cos(radians) * this->y +
                             std::sin(radians) * this->z);
    this->z = static_cast<T>(-std::sin(radians) * this->y +
                             std::cos(radians) * this->z);
}

/// rotate this vector on the Y axis
template <typename T>
inline void vec3<T>::rotateY(D32 radians) {
    this->x = static_cast<T>(std::cos(radians) * this->x -
                             std::sin(radians) * this->z);
    this->z = static_cast<T>(std::sin(radians) * this->x +
                             std::cos(radians) * this->z);
}

/// rotate this vector on the Z axis
template <typename T>
inline void vec3<T>::rotateZ(D32 radians) {
    this->x = static_cast<T>(std::cos(radians) * this->x +
                             std::sin(radians) * this->y);
    this->y = static_cast<T>(-std::sin(radians) * this->x +
                             std::cos(radians) * this->y);
}

/// round all three values
template <typename T>
inline void vec3<T>::round() {
    set(static_cast<T>(std::roundf(this->x)),
        static_cast<T>(std::roundf(this->y)),
        static_cast<T>(std::roundf(this->z)));
}

/// swap the components  of this vector with that of the specified one
template <typename T>
inline void vec3<T>::swap(vec3 &iv) {
    std::swap(this->x, iv.x);
    std::swap(this->y, iv.y);
    std::swap(this->z, iv.z);
}

/// swap the components  of this vector with that of the specified one
template <typename T>
inline void vec3<T>::swap(vec3 *iv) {
    std::swap(this->x, iv->x);
    std::swap(this->y, iv->y);
    std::swap(this->z, iv->z);
}

/// export the vector's components in the first 3 positions of the specified
/// array
template <typename T>
inline void vec3<T>::get(T *v) const {
    v[0] = static_cast<T>(this->_v[0]);
    v[1] = static_cast<T>(this->_v[1]);
    v[2] = static_cast<T>(this->_v[2]);
}

/// this calculates a vector between the two specified points and returns the
/// result
template <typename T>
inline vec3<T> vec3<T>::vector(const vec3 &vp1, const vec3 &vp2) const {
    return vec3(vp1.x - vp2.x, vp1.y - vp2.y, vp1.z - vp2.z);
}

/// return the closest point on the line defined by the 2 points (A, B) and this
/// vector
template <typename T>
inline vec3<T> closestPointOnLine(const vec3<T> &vA, const vec3<T> &vB) {
    return (((vB - vA) * this->projectionOnLine(vA, vB)) + vA);
}

/// return the closest point on the line segment created between the 2 points
/// (A, B) and this vector
template <typename T>
inline vec3<T> closestPointOnSegment(const vec3<T> &vA, const vec3<T> &vB) {
    T factor = this->projectionOnLine(vA, vB);

    if (factor <= 0.0f) return vA;

    if (factor >= 1.0f) return vB;

    return (((vB - vA) * factor) + vA);
}

/// lerp between the 2 specified vectors by the specified amount
template <typename T>
inline vec3<T> lerp(const vec3<T> &u, const vec3<T> &v, T factor) {
    return ((u * (1 - factor)) + (v * factor));
}

/// lerp between the 2 specified vectors by the specified amount for each
/// component
template <typename T>
inline vec3<T> lerp(const vec3<T> &u, const vec3<T> &v, const vec3<T> &factor) {
    return (vec3<T>((u.x * (1 - factor.x)) + (v.x * factor.x),
                    (u.y * (1 - factor.y)) + (v.y * factor.y),
                    (u.z * (1 - factor.z)) + (v.z * factor.z)));
}
/*
*  vec4 inline definitions
*/

/// compare this vector with the one specified and see if they match within the
/// specified amount
template <typename T>
inline bool vec4<T>::compare(const vec4 &v, F32 epsi = EPSILON_F32) const {
    return (FLOAT_COMPARE_TOLERANCE((F32) this->x, (F32)v.x, epsi) &&
            FLOAT_COMPARE_TOLERANCE((F32) this->y, (F32)v.y, epsi) &&
            FLOAT_COMPARE_TOLERANCE((F32) this->z, (F32)v.z, epsi) &&
            FLOAT_COMPARE_TOLERANCE((F32) this->w, (F32)v.w, epsi));
}

/// round all four values
template <typename T>
inline void vec4<T>::round() {
    set((T)std::roundf(this->x), (T)std::roundf(this->y),
        (T)std::roundf(this->z), (T)std::roundf(this->w));
}

/// swap this vector's values with that of the specified vector
template <typename T>
inline void vec4<T>::swap(vec4 *iv) {
    std::swap(this->x, iv->x);
    std::swap(this->y, iv->y);
    std::swap(this->z, iv->z);
    std::swap(this->w, iv->w);
}

/// swap this vector's values with that of the specified vector
template <typename T>
inline void vec4<T>::swap(vec4 &iv) {
    std::swap(this->x, iv.x);
    std::swap(this->y, iv.y);
    std::swap(this->z, iv.z);
    std::swap(this->w, iv.w);
}

/// general vec4 dot product
template <typename T>
inline T dot(const vec4<T> &a, const vec4<T> &b) {
    return (a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
}

/// calculate the dot product between this vector and the specified one
template <typename T>
inline T vec4<T>::dot(const vec4 &v) const {
    return Divide::dot(*this, v);
}

/// return the squared distance of the vector
template <typename T>
inline T vec4<T>::lengthSquared() const {
    return this->x * this->x + this->y * this->y + this->z * this->z +
           this->w * this->w;
}
/// transform the vector to unit length
template <typename T>
inline T vec4<T>::normalize() {
    T l = this->length();

    if (l < EPSILON_F32) return 0;

    // multiply by the inverse length
    *this *= (1.0f / l);

    return l;
}
/// lerp between this and the specified vector by the specified amount
template <typename T>
inline void vec4<T>::lerp(const vec4 &v, T factor) {
    set((*this * (1 - factor)) + (v * factor));
}

/// lerp between this and the specified vector by the specified amount for each
/// component
template <typename T>
inline void vec4<T>::lerp(const vec4 &v, const vec4 &factor) {
    set((*this * (1 - factor)) + (v * factor));
}
/// lerp between the 2 vectors by the specified amount
template <typename T>
inline vec4<T> lerp(const vec4<T> &u, const vec4<T> &v, T factor) {
    return ((u * (1 - factor)) + (v * factor));
}

/// lerp between the 2 specified vectors by the specified amount for each
/// component
template <typename T>
inline vec4<T> lerp(const vec4<T> &u, const vec4<T> &v, const vec4<T> &factor) {
    return (vec4<T>((u.x * (1 - factor.x)) + (v.x * factor.x),
                    (u.y * (1 - factor.y)) + (v.y * factor.y),
                    (u.z * (1 - factor.z)) + (v.z * factor.z),
                    (u.w * (1 - factor.w)) + (v.w * factor.w)));
}

template <typename Type>
inline vec2<Type> random(vec2<Type> min, vec2<Type> max) {
    return vec2<Type>(random(min.x, max.x), random(min.y, max.y));
}

template <typename Type>
inline vec3<Type> random(vec3<Type> min, vec3<Type> max) {
    return vec3<Type>(random(min.x, max.x), random(min.y, max.y),
                      random(min.z, max.z));
}

template <typename Type>
inline vec4<Type> random(vec4<Type> min, vec4<Type> max) {
    return vec4<Type>(random(min.x, max.x), random(min.y, max.y),
                      random(min.z, max.z), random(min.w, max.w));
}
};  // namespace Divide
#endif

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

#ifndef _MATH_MATRICES_INL_
#define _MATH_MATRICES_INL_
namespace Divide {
/*********************************
* mat2
*********************************/
template<typename T>
mat2<T>::mat2()
    : mat2(1, 0,
           0, 1)
{
}

template<typename T>
template<typename U>
mat2<T>::mat2(U m0, U m1,
              U m2, U m3)
    : mat{static_cast<T>(m0), static_cast<T>(m1),
          static_cast<T>(m2), static_cast<T>(m3)}
{
}

template<typename T>
template<typename U>
mat2<T>::mat2(const U *m)
    : mat2(m[0], m[1],
           m[2], m[3])
{
}

template<typename T>
mat2<T>::mat2(const mat2 &m)
    : mat2(m.mat)
{
}

template<typename T>
template<typename U>
mat2<T>::mat2(const mat2<U> &m)
    : mat2(m.mat)
{
}

template<typename T>
template<typename U>
mat2<T>::mat2(const mat3<U> &m)
    : mat2(m[0], m[1],
           m[3], m[4])
{
}

template<typename T>
template<typename U>
mat2<T>::mat2(const mat4<U> &m)
    : mat2(m[0], m[1],
           m[4], m[5])
{
}

template<typename T>
mat2<T>& mat2<T>::operator=(const mat2& other) {
    set(other);
    return *this;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator=(const mat2<U>& other) {
    set(other);
    return *this;
}

template<typename T>
template<typename U>
vec2<T> mat2<T>::operator*(const vec2<U> &v) const {
    return vec2<T>(mat[0] * v[0] + mat[1] * v[1],
                   mat[2] * v[0] + mat[3] * v[1]);
}

template<typename T>
template<typename U>
vec3<T> mat2<T>::operator*(const vec3<U> &v) const {
    return vec3<T>(mat[0] * v[0] + mat[1] * v[1],
                   mat[2] * v[0] + mat[3] * v[1],
                   v[2]);
}

template<typename T>
template<typename U>
vec4<T> mat2<T>::operator*(const vec4<U> &v) const {
    return vec4<T>(mat[0] * v[0] + mat[1] * v[1],
                   mat[2] * v[0] + mat[3] * v[1],
                   v[2],
                   v[3]);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator*(const mat2<U> &m) const {
    return mat2(mat[0] * m.mat[0] + mat[1] * m.mat[2], mat[0] * m.mat[1] + mat[1] * m.mat[3],
                mat[2] * m.mat[0] + mat[3] * m.mat[2], mat[2] * m.mat[1] + mat[3] * m.mat[3]);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator/(const mat2<U> &m) const {
    return this * m.getInverse();
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator+(const mat2<U> &m) const {
    return mat2(mat[0] + m[0], mat[1] + m[1],
                mat[2] + m[2], mat[3] + m[3]);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator-(const mat2<U> &m) const {
    return mat2(mat[0] - m[0], mat[1] - m[1],
                mat[2] - m[2], mat[3] - m[3]);
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator*=(const mat2<U> &m) {
    return *this = *this * m;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator/=(const mat2<U> &m) {
    return *this = *this * m;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator+=(const mat2<U> &m) {
    return *this = *this + m;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator-=(const mat2<U> &m) {
    return *this = *this - m;
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator*(U f) const {
    return mat2(mat[0] * f, mat[1] * f,
                mat[2] * f, mat[3] * f);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator/(U f) const {
    return mat2(mat[0] / f, mat[1] / f,
                mat[2] / f, mat[3] / f);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator+(U f) const {
    return mat2(mat[0] + f, mat[1] + f,
                mat[2] + f, mat[3] + f);
}

template<typename T>
template<typename U>
mat2<T> mat2<T>::operator-(U f) const {
    return mat2(mat[0] - f, mat[1] - f,
                mat[2] - f, mat[3] - f);
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator*=(U f) {
    return *this = *this * f;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator/=(U f) {
    return *this = *this / f;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator+=(U f) {
    return *this = *this + f;
}

template<typename T>
template<typename U>
mat2<T>& mat2<T>::operator-=(U f) {
    return *this = *this - f;
}

template<typename T>
bool mat2<T>::operator==(const mat2 &B) const {
    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1])) {
        return false;
    }

    return true;
}

template<typename T>
bool mat2<T>::operator!=(const mat2 &B) const {
    return !(*this == B);
}

template<typename T>
template<typename U>
bool mat2<T>::operator==(const mat2<U> &B) const {
    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1])) {
        return false;
    }

    return true;
}

template<typename T>
template<typename U>
bool mat2<T>::operator!=(const mat2<U> &B) const {
    return !(*this == B);
}

template<typename T>
mat2<T>::operator T *() {
    return mat;
}

template<typename T>
mat2<T>::operator const T *() const {
    return mat;
}

template<typename T>
T& mat2<T>::operator[](I32 i) {
    return mat[i];
}

template<typename T>
const T mat2<T>::operator[](I32 i) const {
    return mat[i];
}

template<typename T>
T& mat2<T>::element(I8 row, I8 column) {
    return m[row][column];
}

template<typename T>
const T& mat2<T>::element(I8 row, I8 column) const {
    return m[row][column];
}

template<typename T>
template<typename U>
void mat2<T>::set(U m0, U m1, U m2, U m3) {
    mat[0] = static_cast<T>(m0);
    mat[3] = static_cast<T>(m3);
    mat[1] = static_cast<T>(m1);
    mat[2] = static_cast<T>(m2);
}

template<typename T>
template<typename U>
void mat2<T>::set(const U *matrix) {
    static_assert(sizeof(T) == sizeof(U), "unsupported data type");
    memcpy(mat, matrix, sizeof(U) * 4);
}

template<typename T>
template<typename U>
void mat2<T>::set(const mat2<U> &matrix) {
    set(matrix.mat);
}

template<typename T>
template<typename U>
void mat2<T>::set(const mat3<U> &matrix) {
    set(matrix[0], matrix[1], matrix[3], matrix[4]);
}

template<typename T>
template<typename U>
void mat2<T>::set(const mat4<U> &matrix) {
    set(matrix[0], matrix[1], matrix[4], matrix[5]);
}

template<typename T>
void mat2<T>::zero() {
    memset(mat, 0, 4 * sizeof(T));
}

template<typename T>
void mat2<T>::identity() {
    mat[0] = static_cast<T>(1);
    mat[1] = static_cast<T>(0);
    mat[2] = static_cast<T>(0);
    mat[3] = static_cast<T>(1);
}

template<typename T>
bool mat2<T>::isIdentity() const {
    return (COMPARE(mat[0], 1) && IS_ZERO(mat[1]) &&
            IS_ZERO(mat[2])    && COMPARE(mat[3], 1));
}

template<typename T>
void mat2<T>::swap(mat2 &B) {
    std::swap(m[0][0], B.m[0][0]);
    std::swap(m[0][1], B.m[0][1]);

    std::swap(m[1][0], B.m[1][0]);
    std::swap(m[1][1], B.m[1][1]);
}

template<typename T>
T mat2<T>::det() const {
    return (mat[0] * mat[3] - mat[1] * mat[2]);
}

template<typename T>
void mat2<T>::inverse() {
    F32 idet = static_cast<F32>(det());
    assert(!IS_ZERO(idet));
    idet = 1 / idet;

    set( mat[3] * idet, -mat[1] * idet,
        -mat[2] * idet,  mat[0] * idet);
}

template<typename T>
void mat2<T>::transpose() {
    set(mat[0], mat[2],
        mat[1], mat[3]);
}

template<typename T>
void mat2<T>::inverseTranspose() {
    inverse();
    transpose();
}

template<typename T>
mat2<T> mat2<T>::getInverse() const {
    mat2<T> ret(mat);
    ret.inverse();
    return ret;
}

template<typename T>
void mat2<T>::getInverse(mat2<T> &ret) const {
    ret.set(mat);
    ret.inverse();
}

template<typename T>
mat2<T> mat2<T>::getTranspose() const {
    mat2 ret(mat);
    ret.transpose();
    return ret;
}

template<typename T>
void mat2<T>::getTranspose(mat2 &ret) const {
    ret.set(mat);
    ret.transpose();
}

template<typename T>
mat2<T> mat2<T>::getInverseTranspose() const {
    mat2 ret;
    getInverse(ret);
    ret.transpose();
    return ret;
}

template<typename T>
void mat2<T>::getInverseTranspose(mat2 &ret) const {
    getInverse(ret);
    ret.transpose();
}


/*********************************
* mat3
*********************************/

template<typename T>
mat3<T>::mat3()
    : mat3(1, 0, 0,
           0, 1, 0,
           0, 0, 1)
{
}

template<typename T>
template<typename U>
mat3<T>::mat3(U m0, U m1, U m2,
              U m3, U m4, U m5,
              U m6, U m7, U m8)
    : mat{static_cast<T>(m0), static_cast<T>(m1), static_cast<T>(m2),
          static_cast<T>(m3), static_cast<T>(m4), static_cast<T>(m5),
          static_cast<T>(m6), static_cast<T>(m7), static_cast<T>(m8)}
{
}

template<typename T>
template<typename U>
mat3<T>::mat3(const U *m)
    : mat3(m[0], m[1], m[2],
           m[3], m[4], m[5],
           m[6], m[7], m[8])
{
}

template<typename T>
template<typename U>
mat3<T>::mat3(const mat2<U> &m)
    : mat3(m[0], m[1], 0,
           m[2], m[3], 0,
           0,    0,    0) //maybe m[8] should be 1?
{
}

template<typename T>
mat3<T>::mat3(const mat3 &m)
    : mat3(m.mat)
{
}

template<typename T>
template<typename U>
mat3<T>::mat3(const mat3<U> &m)
    : mat3(m.mat)
{
}

template<typename T>
template<typename U>
mat3<T>::mat3(const mat4<U> &m)
    : mat3(m[0], m[1], m[2],
           m[4], m[5], m[6],
           m[8], m[9], m[10])
{
}

template<typename T>
mat3<T>& mat3<T>::operator=(const mat3& other) {
    this->set(other);
    return *this;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator=(const mat3<U>& other) {
    this->set(other);
    return *this;
}

template<typename T>
template<typename U>
vec2<U> mat3<T>::operator*(const vec2<U> &v) const {
    return *this * vec3<U>(v);
}

template<typename T>
template<typename U>
vec3<U> mat3<T>::operator*(const vec3<U> &v) const {
    return vec3<U>(mat[0] * v[0] + mat[3] * v[1] + mat[6] * v[2],
                   mat[1] * v[0] + mat[4] * v[1] + mat[7] * v[2],
                   mat[2] * v[0] + mat[5] * v[1] + mat[8] * v[2]);
}

template<typename T>
template<typename U>
vec4<U> mat3<T>::operator*(const vec4<U> &v) const {
    return vec4<U>(mat[0] * v[0] + mat[3] * v[1] + mat[6] * v[2],
                   mat[1] * v[0] + mat[4] * v[1] + mat[7] * v[2],
                   mat[2] * v[0] + mat[5] * v[1] + mat[8] * v[2],
                   v[3]);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator*(const mat3<U> &m) const {
    return mat3(mat[0] * m[0] + mat[3] * m[1] + mat[6] * m[2],
                mat[1] * m[0] + mat[4] * m[1] + mat[7] * m[2],
                mat[2] * m[0] + mat[5] * m[1] + mat[8] * m[2],
                mat[0] * m[3] + mat[3] * m[4] + mat[6] * m[5],
                mat[1] * m[3] + mat[4] * m[4] + mat[7] * m[5],
                mat[2] * m[3] + mat[5] * m[4] + mat[8] * m[5],
                mat[0] * m[6] + mat[3] * m[7] + mat[6] * m[8],
                mat[1] * m[6] + mat[4] * m[7] + mat[7] * m[8],
                mat[2] * m[6] + mat[5] * m[7] + mat[8] * m[8]);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator/(const mat3<U> &m) const {
    return *this * m.getInverse();
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator+(const mat3<U> &m) const {
    return mat3(mat[0] + m[0], mat[1] + m[1], mat[2] + m[2],
                mat[3] + m[3], mat[4] + m[4], mat[5] + m[5],
                mat[6] + m[6], mat[7] + m[7], mat[8] + m[8]);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator-(const mat3<U> &m) const {
    return mat3(mat[0] - m[0], mat[1] - m[1], mat[2] - m[2],
                mat[3] - m[3], mat[4] - m[4], mat[5] - m[5],
                mat[6] - m[6], mat[7] - m[7], mat[8] - m[8]);
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator*=(const mat3<U> &m) {
    return *this = *this * m;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator/=(const mat3<U> &m) {
    return *this = *this / m;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator+=(const mat3<U> &m) {
    return *this = *this + m;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator-=(const mat3<U> &m) {
    return *this = *this - m;
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator*(U f) const {
    return mat3(mat[0] * f, mat[1] * f, mat[2] * f,
                mat[3] * f, mat[4] * f, mat[5] * f,
                mat[6] * f, mat[7] * f, mat[8] * f);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator/(U f) const {
    return mat3(mat[0] / f, mat[1] / f, mat[2] / f,
                mat[3] / f, mat[4] / f, mat[5] / f,
                mat[6] / f, mat[7] / f, mat[8] / f);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator+(U f) const {
    return mat3(mat[0] + f, mat[1] + f, mat[2] + f,
                mat[3] + f, mat[4] + f, mat[5] + f,
                mat[6] + f, mat[7] + f, mat[8] + f);
}

template<typename T>
template<typename U>
mat3<T> mat3<T>::operator-(U f) const {
    return mat3(mat[0] - f, mat[1] - f, mat[2] - f,
                mat[3] - f, mat[4] - f, mat[5] - f,
                mat[6] - f, mat[7] - f, mat[8] - f);
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator*=(U f) {
    return *this = *this * f;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator/=(U f) {
    return *this = *this / f;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator+=(U f) {
    return *this = *this + f;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator-=(U f) {
    return *this = *this - f;
}

template<typename T>
bool mat3<T>::operator==(const mat3 &B) const {
    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1]) ||
        !COMPARE(m[0][2], B.m[0][2])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1]) ||
        !COMPARE(m[1][2], B.m[1][2])) {
        return false;
    }
    if (!COMPARE(m[2][0], B.m[2][0]) ||
        !COMPARE(m[2][1], B.m[2][1]) ||
        !COMPARE(m[2][2], B.m[2][2])) {
        return false;
    }
    return true;
}

template<typename T>
bool mat3<T>::operator!=(const mat3 &B) const {
    return !(*this == B);
}

template<typename T>
template<typename U>
bool mat3<T>::operator==(const mat3<U> &B) const {
    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1]) ||
        !COMPARE(m[0][2], B.m[0][2])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1]) ||
        !COMPARE(m[1][2], B.m[1][2])) {
        return false;
    }
    if (!COMPARE(m[2][0], B.m[2][0]) ||
        !COMPARE(m[2][1], B.m[2][1]) ||
        !COMPARE(m[2][2], B.m[2][2])) {
        return false;
    }
    return true;
}

template<typename T>
template<typename U>
bool mat3<T>::operator!=(const mat3<U> &B) const {
    return !(*this == B);
}

template<typename T>
mat3<T>::operator T *() {
    return mat;
}

template<typename T>
mat3<T>::operator const T *() const {
    return mat;
}

template<typename T>
T& mat3<T>::operator[](I32 i) {
    return mat[i];
}

template<typename T>
const T mat3<T>::operator[](I32 i) const {
    return mat[i];
}

template<typename T>
T& mat3<T>::element(I8 row, I8 column) {
    return m[row][column];
}

template<typename T>
const T& mat3<T>::element(I8 row, I8 column) const {
    return m[row][column];
}

template<typename T>
template<typename U>
void mat3<T>::set(U m0, U m1, U m2, U m3, U m4, U m5, U m6, U m7, U m8) {
    mat[0] = static_cast<T>(m0);  mat[1] = static_cast<T>(m1); mat[3] = static_cast<T>(m3);
    mat[2] = static_cast<T>(m2);  mat[4] = static_cast<T>(m4); mat[5] = static_cast<T>(m5);
    mat[6] = static_cast<T>(m6);  mat[7] = static_cast<T>(m7); mat[8] = static_cast<T>(m8);
}

template<typename T>
template<typename U>
void mat3<T>::set(const U *matrix) {
    static_assert(sizeof(T) == sizeof(U), "unsupported data type");
    memcpy(mat, matrix, sizeof(U) * 9);
}

template<typename T>
template<typename U>
void mat3<T>::set(const mat2<U> &matrix) {
    const U zero = static_cast<U>(0);
    set(matrix[0], matrix[1], zero,
        matrix[2], matrix[3], zero,
        zero,      zero,      zero); //maybe mat[8] should be 1?
}

template<typename T>
template<typename U>
void mat3<T>::set(const mat3<U> &matrix) {
    set(matrix.mat);
}

template<typename T>
template<typename U>
void mat3<T>::set(const mat4<U> &matrix) {
    set(matrix[0], matrix[1], matrix[2],
        matrix[4], matrix[5], matrix[6],
        matrix[8], matrix[9], matrix[10]);
}

template<typename T>
void mat3<T>::zero() {
    memset(mat, 0, 9 * sizeof(T));
}

template<typename T>
void mat3<T>::identity() {
    const T zero = static_cast<T>(0);
    const T one = static_cast<T>(1);
    mat[0] = one;  mat[1] = zero; mat[2] = zero;
    mat[3] = zero; mat[4] = one;  mat[5] = zero;
    mat[6] = zero; mat[7] = zero; mat[8] = one;
}

template<typename T>
bool mat3<T>::isIdentity() const {
    return (COMPARE(mat[0], 1) && IS_ZERO(mat[1])    && IS_ZERO(mat[2]) &&
            IS_ZERO(mat[3])    && COMPARE(mat[4], 1) && IS_ZERO(mat[5]) &&
            IS_ZERO(mat[6])    && IS_ZERO(mat[7])    && COMPARE(mat[8], 1));
}

template<typename T>
void mat3<T>::swap(mat3 &B) {
    std::swap(m[0][0], B.m[0][0]);
    std::swap(m[0][1], B.m[0][1]);
    std::swap(m[0][2], B.m[0][2]);

    std::swap(m[1][0], B.m[1][0]);
    std::swap(m[1][1], B.m[1][1]);
    std::swap(m[1][2], B.m[1][2]);

    std::swap(m[2][0], B.m[2][0]);
    std::swap(m[2][1], B.m[2][1]);
    std::swap(m[2][2], B.m[2][2]);
}

template<typename T>
T mat3<T>::det() const {
    return ((mat[0] * mat[4] * mat[8]) +
            (mat[3] * mat[7] * mat[2]) +
            (mat[6] * mat[1] * mat[5]) -
            (mat[6] * mat[4] * mat[2]) -
            (mat[3] * mat[1] * mat[8]) -
            (mat[0] * mat[7] * mat[5]));
}

template<typename T>
void mat3<T>::inverse() {
    F32 idet = det();
    assert(!IS_ZERO(idet));
    idet = 1 / idet;

    set( (mat[4] * mat[8] - mat[7] * mat[5]) * idet,
        -(mat[1] * mat[8] - mat[7] * mat[2]) * idet,
         (mat[1] * mat[5] - mat[4] * mat[2]) * idet,
        -(mat[3] * mat[8] - mat[6] * mat[5]) * idet,
         (mat[0] * mat[8] - mat[6] * mat[2]) * idet,
        -(mat[0] * mat[5] - mat[3] * mat[2]) * idet,
         (mat[3] * mat[7] - mat[6] * mat[4]) * idet,
        -(mat[0] * mat[7] - mat[6] * mat[1]) * idet,
         (mat[0] * mat[4] - mat[3] * mat[1]) * idet);
}

template<typename T>
void mat3<T>::transpose() {
    set(mat[0], mat[3], mat[6],
        mat[1], mat[4], mat[7],
        mat[2], mat[5], mat[8]);
}

template<typename T>
void mat3<T>::inverseTranspose() {
    inverse();
    transpose();
}

template<typename T>
mat3<T> mat3<T>::getInverse() const {
    mat3<T> ret(mat);
    ret.inverse();
    return ret;
}

template<typename T>
void mat3<T>::getInverse(mat3<T> &ret) const {
    ret.set(mat);
    ret.inverse();
}

template<typename T>
mat3<T> mat3<T>::getTranspose() const {
    mat3<T> ret(mat);
    ret.transpose();
    return ret;
}

template<typename T>
void mat3<T>::getTranspose(mat3<T> &ret) const {
    ret.set(mat);
    ret.transpose();
}

template<typename T>
mat3<T> mat3<T>::getInverseTranspose() const {
    mat3<T> ret(getInverse());
    ret.transpose();
    return ret;
}

template<typename T>
void mat3<T>::getInverseTranspose(mat3<T> &ret) const {
    ret.set(this);
    ret.inverseTranspose();
}

template<typename T>
template<typename U>
void mat3<T>::fromRotation(const vec3<U> &v, U angle, bool inDegrees = true) {
    fromRotation(v.x, v.y, v.z, angle, inDegrees);
}

template<typename T>
template<typename U>
void mat3<T>::fromRotation(U x, U y, U z, U angle, bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);
    U l = static_cast<U>(std::sqrt(static_cast<D64>(x * x + y * y + z * z)));

    l = l < EPSILON_F32 ? 1 : 1 / l;
    x *= l;
    y *= l;
    z *= l;

    U xy = x * y;
    U yz = y * z;
    U zx = z * x;
    U xs = x * s;
    U ys = y * s;
    U zs = z * s;
    U c1 = 1 - c;

    set(c1 * x * x + c, c1 * xy + zs,   c1 * zx - ys,
        c1 * xy - zs,   c1 * y * y + c, c1 * yz + xs;
        c1 * zx + ys,   c1 * yz - xs;   c1 * z * z + c);
}

template<typename T>
template<typename U>
void mat3<T>::fromXRotation(U angle, bool inDegrees = true) {
    const U zero = static_cast<U>(0);
    const U one = static_cast<U>(1);

    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    set(one, zero, zero,
        zero, c,   s,
        zero, -s,  c);
}

template<typename T>
template<typename U>
void mat3<T>::fromYRotation(U angle, bool inDegrees = true) {
    const U zero = static_cast<U>(0);
    const U one = static_cast<U>(1);

    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    set(c,    zero, -s,
        zero, one,   zero,
        s,    zero,  c);
}

template<typename T>
template<typename U>
void mat3<T>::fromZRotation(U angle, bool inDegrees = true) {
    const U zero = static_cast<U>(0);
    const U one = static_cast<U>(1);

    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    set( c,    s,    zero,
        -s,    c,    zero,
         zero, zero, one);
}

// setScale replaces the main diagonal!
template<typename T>
template<typename U>
void mat3<T>::setScale(U x, U y, U z) {
    mat[0] = static_cast<T>(x);
    mat[4] = static_cast<T>(y);
    mat[8] = static_cast<T>(z);
}

template<typename T>
template<typename U>
void mat3<T>::setScale(const vec3<U> &v) {
    setScale(v.x, v.y, v.z);
}

template<typename T>
void mat3<T>::orthonormalize(void) {
    vec3<T> x(mat[0], mat[1], mat[2]);
    x.normalize();
    vec3<T> y(mat[3], mat[4], mat[5]);
    vec3<T> z(cross(x, y));
    z.normalize();
    y.cross(z, x);
    y.normalize();

    set(x.x, x.y, x.z,
        y.x, y.y, y.z,
        z.x, z.y, z.z);
}

}; //namespace Divide
#endif //_MATH_MATRICES_INL_

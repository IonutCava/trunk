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
    if(sizeof(T) == sizeof(U)) {
        memcpy(mat, matrix, sizeof(U) * 4);
    } else {
        set(matrix[0], matrix[1],
            matrix[2], matrix[3]);
    }
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
template<typename U>
void mat2<T>::setRow(I32 index, const U value) {
    m[index][0] = static_cast<T>(value);
    m[index][1] = static_cast<T>(value);
}

template<typename T>
template<typename U>
void mat2<T>::setRow(I32 index, const vec2<U> &value) {
    m[index][0] = static_cast<T>(value.x);
    m[index][1] = static_cast<T>(value.y);
}

template<typename T>
template<typename U>
void mat2<T>::setRow(I32 index, const U x, const U y) {
    m[index][0] = static_cast<T>(x);
    m[index][1] = static_cast<T>(y);
}

template<typename T>
template<typename U>
void mat2<T>::setCol(I32 index, const vec2<U> &value) {
    m[0][index] = static_cast<T>(value.x);
    m[1][index] = static_cast<T>(value.y);
}

template<typename T>
template<typename U>
void mat2<T>::setCol(I32 index, const U value) {
    m[0][index] = static_cast<T>(value);
    m[1][index] = static_cast<T>(value);
}

template<typename T>
template<typename U>
void mat2<T>::setCol(I32 index, const U x, const U y) {
    m[0][index] = static_cast<T>(x);
    m[1][index] = static_cast<T>(y);
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
T mat2<T>::elementSum() const {
    return mat[0] + mat[1] + mat[2] + mat[3];
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
    set(other);
    return *this;
}

template<typename T>
template<typename U>
mat3<T>& mat3<T>::operator=(const mat3<U>& other) {
    set(other);
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
    if (sizeof(T) == sizeof(U)) {
        memcpy(mat, matrix, sizeof(U) * 9);
    } else {
        set(matrix[0], matrix[1], matrix[2],
            matrix[3], matrix[4], matrix[5],
            matrix[6], matrix[7], matrix[8]);
    }
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
template<typename U>
void mat3<T>::setRow(I32 index, const U value) {
    m[index][0] = static_cast<T>(value);
    m[index][1] = static_cast<T>(value);
    m[index][2] = static_cast<T>(value);
}

template<typename T>
template<typename U>
void mat3<T>::setRow(I32 index, const vec3<U> &value) {
    m[index][0] = static_cast<T>(value.x);
    m[index][1] = static_cast<T>(value.y);
    m[index][2] = static_cast<T>(value.z);
}

template<typename T>
template<typename U>
void mat3<T>::setRow(I32 index, const U x, const U y, const U z) {
    m[index][0] = static_cast<T>(x);
    m[index][1] = static_cast<T>(y);
    m[index][2] = static_cast<T>(z);
}

template<typename T>
template<typename U>
void mat3<T>::setCol(I32 index, const vec3<U> &value) {
    m[0][index] = static_cast<T>(value.x);
    m[1][index] = static_cast<T>(value.y);
    m[2][index] = static_cast<T>(value.z);
}

template<typename T>
template<typename U>
void mat3<T>::setCol(I32 index, const U value) {
    m[0][index] = static_cast<T>(value);
    m[1][index] = static_cast<T>(value);
    m[2][index] = static_cast<T>(value);
}

template<typename T>
template<typename U>
void mat3<T>::setCol(I32 index, const U x, const U y, const U z) {
    m[0][index] = static_cast<T>(x);
    m[1][index] = static_cast<T>(y);
    m[2][index] = static_cast<T>(z);
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
T mat3<T>::elementSum() const {
    return mat[0] + mat[1] + mat[2] +
           mat[3] + mat[4] + mat[5] +
           mat[6] + mat[7] + mat[8];
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

/***************
* mat4
***************/

template<typename T>
mat4<T>::mat4()
    : mat4(1, 0, 0, 0,
           0, 1, 0, 0,
           0, 0, 1, 0,
           0, 0, 0, 1)
{
}


template<typename T>
template<typename U>
mat4<T>::mat4(U m0, U m1, U m2, U m3,
              U m4, U m5, U m6, U m7,
              U m8, U m9, U m10, U m11,
              U m12, U m13, U m14, U m15)
    : mat{ static_cast<T>(m0),  static_cast<T>(m1),  static_cast<T>(m2),  static_cast<T>(m3),
           static_cast<T>(m4),  static_cast<T>(m5),  static_cast<T>(m6),  static_cast<T>(m7),
           static_cast<T>(m8),  static_cast<T>(m9),  static_cast<T>(m10), static_cast<T>(m11),
           static_cast<T>(m12), static_cast<T>(m13), static_cast<T>(m14), static_cast<T>(m15) }
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const U *m)
    : mat4(m[0], m[1], m[2], m[3],
           m[4], m[5], m[6], m[7],
           m[8], m[9], m[10], m[11],
           m[12], m[13], m[14], m[15])
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const mat2<U> &m)
    : mat4(m[0],              m[1],              static_cast<U>(0), static_cast<U>(0),
           m[2],              m[3],              static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0)) //maybe m[15] should be 1?
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const mat3<U> &m)
    : mat4(m[0],              m[1],             m[2],               static_cast<U>(0),
           m[3],              m[4],              m[5]               static_cast<U>(0),
           m[6],              m[7],              m[8],              static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0)) //maybe m[15] should be 1?
{
}

template<typename T>
mat4<T>::mat4(const mat4 &m) 
    : mat4(m.mat)
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const mat4<U> &m)
    : mat4(m.mat)
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const vec3<U> &translation, const vec3<U> &scale)
    : mat4(scale.x,           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), scale.y,           static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), scale.z,           static_cast<U>(0),
           translation.x,     translation.y,     translation.z,     static_cast<U>(1))
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const vec3<U> &translation, const vec3<U> &scale, const mat4<U>& rotation)
    : mat4(scale.x,           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), scale.y,           static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), scale.z,           static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(1))
{
    set(*this * rotation);
    setTranslation(translation);
}

template<typename T>
template<typename U>
mat4<T>::mat4(const vec3<U> &translation)
    : mat4(translation.x, translation.y, translation.z)
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(U translationX, U translationY, U translationZ)
    : mat4(static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           static_cast<U>(0), static_cast<U>(0), static_cast<U>(0), static_cast<U>(0),
           translationX,      translationY,      translationZ,      static_cast<U>(1))
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(const vec3<U> &axis, U angle, bool inDegrees = true)
    : mat4(axis.x, axis.y, axis.z, angle, inDegrees)
{
}

template<typename T>
template<typename U>
mat4<T>::mat4(U x, U y, U z, U angle, bool inDegrees = true)
    : mat4()
{
    fromRotation(x, y, z, angle, inDegrees);
}

template<typename T>
template<typename U>
mat4<T>::mat4(const vec3<U> &eye, const vec3<U> &target, const vec3<U> &up)
    : mat4()
{
    lookAt(eye, target, up);
}


template<typename T>
mat4<T>& mat4<T>::operator=(const mat4& other) {
    set(other);
    return *this;
}
template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator=(const mat4<U>& other) {
    set(other);
    return *this;
}

template<typename T>
template<typename U>
vec2<U> mat4<T>::operator*(const vec2<U> &v) const {
    return *this * vec4<U>(v);
}

template<typename T>
template<typename U>
vec3<U> mat4<T>::operator*(const vec3<U> &v) const {
    return *this * vec4<U>(v);
}

template<typename T>
template<typename U>
vec4<U> mat4<T>::operator*(const vec4<U> &v) const {
    return vec4<U>(mat[0] * v.x + mat[4] * v.y + mat[8]  * v.z + mat[12] * v.w,
                   mat[1] * v.x + mat[5] * v.y + mat[9]  * v.z + mat[13] * v.w,
                   mat[2] * v.x + mat[6] * v.y + mat[10] * v.z + mat[14] * v.w,
                   mat[3] * v.x + mat[7] * v.y + mat[11] * v.z + mat[15] * v.w);
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator*(const mat4<U>& matrix) const {
    mat4<T> retValue;
    Multiply(*this, matrix, retValue);
    return retValue;
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator/(const mat4<U>& matrix) const {
    mat4<T> retValue;
    Multiply(*this, matrix.getInverse(), retValue);
    return retValue;
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator+(const mat4<U> &matrix) const {
    const U* b = matrix.mat;

    return mat4<T>(mat[0]  + static_cast<T>(b[0]),  mat[1]  + static_cast<T>(b[1]),  mat[2]  + static_cast<T>(b[2]),  mat[3]  + static_cast<T>(b[3]),
                   mat[4]  + static_cast<T>(b[4]),  mat[5]  + static_cast<T>(b[5]),  mat[6]  + static_cast<T>(b[6]),  mat[7]  + static_cast<T>(b[7]), 
                   mat[8]  + static_cast<T>(b[8]),  mat[9]  + static_cast<T>(b[9]),  mat[10] + static_cast<T>(b[10]), mat[11] + static_cast<T>(b[11]), 
                   mat[12] + static_cast<T>(b[12]), mat[13] + static_cast<T>(b[13]), mat[14] + static_cast<T>(b[14]), mat[15] + static_cast<T>(b[15]));
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator-(const mat4<U> &matrix) const {
    const U* b = matrix.mat;

    return mat4<T>(mat[0]  - static_cast<T>(b[0]),  mat[1]  - static_cast<T>(b[1]),  mat[2]  - static_cast<T>(b[2]),  mat[3]  - static_cast<T>(b[3]),
                   mat[4]  - static_cast<T>(b[4]),  mat[5]  - static_cast<T>(b[5]),  mat[6]  - static_cast<T>(b[6]),  mat[7]  - static_cast<T>(b[7]),
                   mat[8]  - static_cast<T>(b[8]),  mat[9]  - static_cast<T>(b[9]),  mat[10] - static_cast<T>(b[10]), mat[11] - static_cast<T>(b[11]),
                   mat[12] - static_cast<T>(b[12]), mat[13] - static_cast<T>(b[13]), mat[14] - static_cast<T>(b[14]), mat[15] - static_cast<T>(b[15]));
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator*=(const mat4<U> &matrix) {
    Multiply(*this, matrix, *this);
    return *this;
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator/=(const mat4<U> &matrix) {
    Multiply(*this, matrix.getInverse(), *this);
    return *this;
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator+=(const mat4<U> &matrix) {
    const U* b = matrix.mat;

    U8 index = 0;
    for (U8 i = 0; i < 4; ++i) {
        index = i * 4;
        setRow(i, mat[index + 0] + static_cast<T>(b[index + 0]),
                  mat[index + 1] + static_cast<T>(b[index + 1]),
                  mat[index + 2] + static_cast<T>(b[index + 2]),
                  mat[index + 3] + static_cast<T>(b[index + 3]));
    }
    return *this;
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator-=(const mat4<U> &matrix) {
    const U* b = matrix.mat;

    U8 index = 0;
    for (U8 i = 0; i < 4; ++i) {
        index = i * 4;
        setRow(i, mat[index + 0] - static_cast<T>(b[index + 0]),
                  mat[index + 1] - static_cast<T>(b[index + 1]),
                  mat[index + 2] - static_cast<T>(b[index + 2]),
                  mat[index + 3] - static_cast<T>(b[index + 3]));
    }

    return *this;
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator*(U f) const {
    return mat4<T>(mat[0]  * f, mat[1]  * f, mat[2]  * f, mat[3]  * f,
                   mat[4]  * f, mat[5]  * f, mat[6]  * f, mat[7]  * f,
                   mat[8]  * f, mat[9]  * f, mat[10] * f, mat[11] * f,
                   mat[12] * f, mat[13] * f, mat[14] * f, mat[15] * f);
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator/(U f) const {
    return mat4<T>(mat[0]  / f, mat[1]  / f, mat[2]  / f, mat[3]  / f,
                   mat[4]  / f, mat[5]  / f, mat[6]  / f, mat[7]  / f,
                   mat[8]  / f, mat[9]  / f, mat[10] / f, mat[11] / f,
                   mat[12] / f, mat[13] / f, mat[14] / f, mat[15] / f);
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator+(U f) const {
    return mat4(mat[0]  + f, mat[1]  + f, mat[2]  + f, mat[3]  + f,
                mat[4]  + f, mat[5]  + f, mat[6]  + f, mat[7]  + f,
                mat[8]  + f, mat[9]  + f, mat[10] + f, mat[11] + f,
                mat[12] + f, mat[13] + f, mat[14] + f, mat[15] + f);
}

template<typename T>
template<typename U>
mat4<T> mat4<T>::operator-(U f) const {
    return mat4(mat[0]  - f, mat[1]  - f, mat[2]  - f, mat[3]  - f,
                mat[4]  - f, mat[5]  - f, mat[6]  - f, mat[7]  - f,
                mat[8]  - f, mat[9]  - f, mat[10] - f, mat[11] - f,
                mat[12] - f, mat[13] - f, mat[14] - f, mat[15] - f);
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator*=(U f) {
    U8 index = 0;
    for (U8 i = 0; i < 4; ++i) {
        index = i * 4;
        setRow(i, mat[index + 0] * f,
                  mat[index + 1] * f,
                  mat[index + 2] * f,
                  mat[index + 3] * f);
    }

    return *this;
}

template<typename T>
template<typename U>
mat4<T>& mat4<T>::operator/=(U f) {
    U8 index = 0;
    for (U8 i = 0; i < 4; ++i) {
        index = i * 4;
        setRow(i, mat[index + 0] / f,
                  mat[index + 1] / f,
                  mat[index + 2] / f,
                  mat[index + 3] / f);
    }

    return *this;
}

template<typename T>
bool mat4<T>::operator==(const mat4& B) const {
    // Add a small epsilon value to avoid 0.0 != 0.0
    if (!COMPARE(elementSum() + EPSILON_F32,
        B.elementSum() + EPSILON_F32)) {
        return false;
    }

    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1]) ||
        !COMPARE(m[0][2], B.m[0][2]) ||
        !COMPARE(m[0][3], B.m[0][3])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1]) ||
        !COMPARE(m[1][2], B.m[1][2]) ||
        !COMPARE(m[1][3], B.m[1][3])) {
        return false;
    }
    if (!COMPARE(m[2][0], B.m[2][0]) ||
        !COMPARE(m[2][1], B.m[2][1]) ||
        !COMPARE(m[2][2], B.m[2][2]) ||
        !COMPARE(m[2][3], B.m[2][3])) {
        return false;
    }
    if (!COMPARE(m[3][0], B.m[3][0]) ||
        !COMPARE(m[3][1], B.m[3][1]) ||
        !COMPARE(m[3][2], B.m[3][2]) ||
        !COMPARE(m[3][3], B.m[3][3])) {
        return false;
    }

    return true;
}

template<typename T>
bool mat4<T>::operator!=(const mat4 &B) const {
    return !(*this == B);
}

template<typename T>
template<typename U>
bool mat4<T>::operator==(const mat4<U>& B) const {
    // Add a small epsilon value to avoid 0.0 != 0.0
    if (!COMPARE(elementSum() + EPSILON_F32,
        B.elementSum() + EPSILON_F32)) {
        return false;
    }

    if (!COMPARE(m[0][0], B.m[0][0]) ||
        !COMPARE(m[0][1], B.m[0][1]) ||
        !COMPARE(m[0][2], B.m[0][2]) ||
        !COMPARE(m[0][3], B.m[0][3])) {
        return false;
    }
    if (!COMPARE(m[1][0], B.m[1][0]) ||
        !COMPARE(m[1][1], B.m[1][1]) ||
        !COMPARE(m[1][2], B.m[1][2]) ||
        !COMPARE(m[1][3], B.m[1][3])) {
        return false;
    }
    if (!COMPARE(m[2][0], B.m[2][0]) ||
        !COMPARE(m[2][1], B.m[2][1]) ||
        !COMPARE(m[2][2], B.m[2][2]) ||
        !COMPARE(m[2][3], B.m[2][3])) {
        return false;
    }
    if (!COMPARE(m[3][0], B.m[3][0]) ||
        !COMPARE(m[3][1], B.m[3][1]) ||
        !COMPARE(m[3][2], B.m[3][2]) ||
        !COMPARE(m[3][3], B.m[3][3])) {
        return false;
    }

    return true;
}

template<typename T>
template<typename U>
bool mat4<T>::operator!=(const mat4<U> &B) const {
    return !(*this == B);
}

template<typename T>
mat4<T>::operator T *() {
    return mat;
}

template<typename T>
mat4<T>::operator const T *() const {
    return mat;
}

template<typename T>
T& mat4<T>::operator[](I32 i) {
    return mat[i];
}

template<typename T>
const T& mat4<T>::operator[](I32 i) const {
    return mat[i];
}

template<typename T>
T& mat4<T>::element(I8 row, I8 column) {
    return m[row][column];
}

template<typename T>
const T& mat4<T>::element(I8 row, I8 column) const {
    return m[row][column];
}

template<typename T>
template<typename U>
void mat4<T>::set(U m0, U m1, U m2, U m3, U m4, U m5, U m6, U m7, U m8, U m9, U m10, U m11, U m12, U m13, U m14, U m15) {
    mat[0]  = static_cast<T>(m0);
    mat[4]  = static_cast<T>(m4);
    mat[8]  = static_cast<T>(m8);
    mat[12] = static_cast<T>(m12);

    mat[1]  = static_cast<T>(m1);
    mat[5]  = static_cast<T>(m5);
    mat[9]  = static_cast<T>(m9);
    mat[13] = static_cast<T>(m13);

    mat[2]  = static_cast<T>(m2);
    mat[6]  = static_cast<T>(m6);
    mat[10] = static_cast<T>(m10);
    mat[14] = static_cast<T>(m14);

    mat[3]  = static_cast<T>(m3);
    mat[7]  = static_cast<T>(m7);
    mat[11] = static_cast<T>(m11);
    mat[15] = static_cast<T>(m15);
}

template<typename T>
template<typename U>
void mat4<T>::set(U const *matrix) {
    if (sizeof(T) == sizeof(U)) {
        memcpy(mat, matrix, sizeof(U) * 16);
    } else {
        set(matrix[0],  matrix[1],  matrix[2],  matrix[3],
            matrix[4],  matrix[5],  matrix[6],  matrix[7],
            matrix[8],  matrix[9],  matrix[10], matrix[11],
            matrix[12], matrix[13], matrix[14], matrix[15]);
    }
}

template<typename T>
template<typename U>
void mat4<T>::set(const mat2<U> &matrix) {
    const U zero = static_cast<U>(0);

    set(matrix[0], matrix[1], zero, zero,
        matrix[2], matrix[3], zero, zero,
        zero,      zero,      zero, zero,
        zero,      zero,      zero, zero);
}

template<typename T>
template<typename U>
void mat4<T>::set(const mat3<U> &matrix) {
    const U zero = static_cast<U>(0);

    set(matrix[0], matrix[1], matrix[2], zero,
        matrix[3], matrix[4], matrix[5], zero,
        matrix[6], matrix[7], matrix[8], zero,
        zero,      zero,      zero,      zero);
}

template<typename T>
template<typename U>
void mat4<T>::set(const mat4<U> &matrix) {
    set(matrix.mat);
}

template<typename T>
template<typename U>
void mat4<T>::setRow(I32 index, const U value) {
    _vec[index].set(value);
}

template<typename T>
template<typename U>
void mat4<T>::setRow(I32 index, const vec4<U> &value) {
    _vec[index].set(value);
}

template<typename T>
template<typename U>
void mat4<T>::setRow(I32 index, const U x, const U y, const U z, const U w) {
    _vec[index].set(x, y, z, w);
}

template<typename T>
template<typename U>
void mat4<T>::setCol(I32 index, const vec4<U> &value) {
    setCol(index, value.x, value.y, value.z, value.w);
}

template<typename T>
template<typename U>
void mat4<T>::setCol(I32 index, const U value) {
    m[0][index] = static_cast<T>(value);
    m[1][index] = static_cast<T>(value);
    m[2][index] = static_cast<T>(value);
    m[3][index] = static_cast<T>(value);
}

template<typename T>
template<typename U>
void mat4<T>::setCol(I32 index, const U x, const U y, const U z, const U w) {
    m[0][index] = static_cast<T>(x);
    m[1][index] = static_cast<T>(y);
    m[2][index] = static_cast<T>(z);
    m[3][index] = static_cast<T>(w);
}

template<typename T>
void mat4<T>::zero() {
    memset(mat, 0, sizeof(T) * 16);
}

template<typename T>
void mat4<T>::identity() {
    set(1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
}

template<typename T>
bool mat4<T>::isIdentity() const {
    return (COMPARE(mat[0], 1) && IS_ZERO(mat[1])    && IS_ZERO(mat[2])     && IS_ZERO(mat[3])
            IS_ZERO(mat[4])    && COMPARE(mat[5], 1) && IS_ZERO(mat[6])     && IS_ZERO(mat[7])
            IS_ZERO(mat[8])    && IS_ZERO(mat[9])    && COMPARE(mat[10], 1) && IS_ZERO(mat[11]),
            IS_ZERO(mat[12])   && IS_ZERO(mat[13])   && IS_ZERO(mat[14])    && COMPARE(mat[15], 1));
}

template<typename T>
void mat4<T>::swap(mat4 &B) {
    std::swap(m[0][0], B.m[0][0]);
    std::swap(m[0][1], B.m[0][1]);
    std::swap(m[0][2], B.m[0][2]);
    std::swap(m[0][3], B.m[0][3]);

    std::swap(m[1][0], B.m[1][0]);
    std::swap(m[1][1], B.m[1][1]);
    std::swap(m[1][2], B.m[1][2]);
    std::swap(m[1][3], B.m[1][3]);

    std::swap(m[2][0], B.m[2][0]);
    std::swap(m[2][1], B.m[2][1]);
    std::swap(m[2][2], B.m[2][2]);
    std::swap(m[2][3], B.m[2][3]);

    std::swap(m[3][0], B.m[3][0]);
    std::swap(m[3][1], B.m[3][1]);
    std::swap(m[3][2], B.m[3][2]);
    std::swap(m[3][3], B.m[3][3]);
}

template<typename T>
T mat4<T>::det() const {
    return ((mat[0] * mat[5] * mat[10]) + (mat[4] * mat[9] * mat[2]) +
            (mat[8] * mat[1] * mat[6])  - (mat[8] * mat[5] * mat[2]) -
            (mat[4] * mat[1] * mat[10]) - (mat[0] * mat[9] * mat[6]));
}

template<typename T>
T mat4<T>::elementSum() const {
    return mat[0]  + mat[1]  + mat[2]  + mat[3]  +
           mat[4]  + mat[5]  + mat[6]  + mat[7]  +
           mat[8]  + mat[9]  + mat[10] + mat[11] +
           mat[12] + mat[13] + mat[14] + mat[15];
}

template<typename T>
void mat4<T>::inverse() {
    Inverse(mat, mat);
}

template<typename T>
void mat4<T>::transpose() {
    set(mat[0], mat[4], mat[8],  mat[12],
        mat[1], mat[5], mat[9],  mat[13],
        mat[2], mat[6], mat[10], mat[14],
        mat[3], mat[7], mat[11], mat[15]);
}

template<typename T>
void mat4<T>::inverseTranspose() {
    inverse();
    transpose();
}

template<typename T>
mat4<T> mat4<T>::transposeRotation() const {
    set(mat[0], mat[4], mat[8], mat[3],
        mat[1], mat[5], mat[9], mat[7],
        mat[2], mat[6], mat[10], mat[11],
        mat[12], mat[13], mat[14], mat[15]);
}

template<typename T>
mat4<T> mat4<T>::getInverse() const {
    mat4 ret;
    Inverse(mat, ret.mat);
    return ret;
}

template<typename T>
void mat4<T>::getInverse(mat4 &ret) const {
    Inverse(mat, ret.mat);
}

template<typename T>
mat4<T> mat4<T>::getTranspose() const {
    return mat4(mat[0], mat[4], mat[8],  mat[12],
                mat[1], mat[5], mat[9],  mat[13],
                mat[2], mat[6], mat[10], mat[14],
                mat[3], mat[7], mat[11], mat[15]);
}

template<typename T>
void mat4<T>::getTranspose(mat4 &out) const {
    out.set(mat[0], mat[4], mat[8],  mat[12],
            mat[1], mat[5], mat[9],  mat[13],
            mat[2], mat[6], mat[10], mat[14],
            mat[3], mat[7], mat[11], mat[15]);
}

template<typename T>
mat4<T> mat4<T>::getInverseTranspose() const {
    mat4 ret;
    Inverse(mat, ret.mat);
    ret.transpose();
    return ret;
}

template<typename T>
void mat4<T>::getInverseTranspose(mat4 &ret) const {
    Inverse(mat, ret.mat);
    ret.transpose();
}

template<typename T>
mat4<T> mat4<T>::getTransposeRotation() const {
    return mat4(mat[0],  mat[4],  mat[8],  mat[3],
                mat[1],  mat[5],  mat[9],  mat[7],
                mat[2],  mat[6],  mat[10], mat[11],
                mat[12], mat[13], mat[14], mat[15]);
}

template<typename T>
void mat4<T>::getTransposeRotation(mat4 &ret) const {
    ret.set(mat[0],  mat[4],  mat[8],  mat[3],
            mat[1],  mat[5],  mat[9],  mat[7],
            mat[2],  mat[6],  mat[10], mat[11],
            mat[12], mat[13], mat[14], mat[15]);
}

template<typename T>
template<typename U>
void mat4<T>::fromRotation(U x, U y, U z, U angle, bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    vec3<U> v(x, y, z);
    v.normalize();

    U c = std::cos(angle);
    U s = std::sin(angle);

    U xx = v.x * v.x;
    U yy = v.y * v.y;
    U zz = v.z * v.z;
    U xy = v.x * v.y;
    U yz = v.y * v.z;
    U zx = v.z * v.x;
    U xs = v.x * s;
    U ys = v.y * s;
    U zs = v.z * s;

    set((1 - c) * xx + c,        (1 - c) * xy + zs,       (1 - c) * zx - ys,       static_cast<U>(mat[3]),
        (1 - c) * xy - zs,       (1 - c) * yy + c,        (1 - c) * yz + xs,       static_cast<U>(mat[7]),
        (1 - c) * zx + ys,       (1 - c) * yz - xs,       (1 - c) * zz + c,        static_cast<U>(mat[11]),
        static_cast<U>(mat[12]), static_cast<U>(mat[13]), static_cast<U>(mat[14]), static_cast<U>(mat[15]))
}

template<typename T>
template<typename U>
void mat4<T>::fromXRotation(U angle, bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    mat[5] = static_cast<T>(c);
    mat[9] = static_cast<T>(-s);
    mat[6] = static_cast<T>(s);
    mat[10] = static_cast<T>(c);
}

template<typename T>
template<typename U>
void mat4<T>::fromYRotation(U angle, bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    mat[0] = static_cast<T>(c);
    mat[8] = static_cast<T>(s);
    mat[2] = static_cast<T>(-s);
    mat[10] = static_cast<T>(c);
}

template<typename T>
template<typename U>
void mat4<T>::fromZRotation(U angle, bool inDegrees = true) {
    if (inDegrees) {
        angle = Angle::DegreesToRadians(angle);
    }

    U c = std::cos(angle);
    U s = std::sin(angle);

    mat[0] = static_cast<T>(c);
    mat[4] = static_cast<T>(-s);
    mat[1] = static_cast<T>(s);
    mat[5] = static_cast<T>(c);
}

template<typename T>
template<typename U>
void mat4<T>::setTranslation(const vec3<U> &v) {
    setTranslation(v.x, v.y, v.z);
}

template<typename T>
template<typename U>
void mat4<T>::setTranslation(U x, U y, U z) {
    mat[12] = static_cast<T>(x);
    mat[13] = static_cast<T>(y);
    mat[14] = static_cast<T>(z);
}

template<typename T>
template<typename U>
void mat4<T>::setScale(U x, U y, U z) {
    mat[0]  = static_cast<T>(x);
    mat[5]  = static_cast<T>(y);
    mat[10] = static_cast<T>(z);
}

template<typename T>
template<typename U>
void mat4<T>::setScale(const vec3<U> &v) {
    setScale(v.x, v.y, v.z);
}

template<typename T>
template<typename U>
vec3<U> mat4<T>::transform(const vec3<U> &v, bool homogeneous) const {
    return  homogeneous ? transformHomogeneous(v)
                        : transformNonHomogeneous(v);
}

template<typename T>
template<typename U>
vec3<U> mat4<T>::transformHomogeneous(const vec3<U> &v) const {
    //Transforms the given 3-D vector by the matrix, projecting the result back into <i>w</i> = 1. (OGRE reference)
    F32 fInvW = 1.0f / (m[0][3] * v.x + m[1][3] * v.y +
                        m[2][3] * v.z + m[3][3]);

    return vec3<U>((m[0][0] * v.x + m[1][1] * v.y + m[2][0] * v.z + m[3][0]) * fInvW,
                   (m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1]) * fInvW,
                   (m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2]) * fInvW);
}

template<typename T>
template<typename U>
vec3<U> mat4<T>::transformNonHomogeneous(const vec3<U> &v) const {
    return *this * vec4<U>(v, static_cast<U>(0));
}

template<typename T>
template<typename U>
void mat4<T>::translate(const vec3<U> &v) {
    translate(v.x, v.y, v.z);
}

template<typename T>
template<typename U>
void mat4<T>::translate(U x, U y, U z) {
    mat[12] += static_cast<T>(x);
    mat[13] += static_cast<T>(y);
    mat[14] += static_cast<T>(z);
}

template<typename T>
template<typename U>
void mat4<T>::scale(const vec3<U> &v) {
    scale(v.x, v.y, v.z);
}

template<typename T>
template<typename U>
void mat4<T>::scale(U x, U y, U z) {
    mat[0]  *= static_cast<T>(x);
    mat[4]  *= static_cast<T>(y);
    mat[8]  *= static_cast<T>(z);
    mat[1]  *= static_cast<T>(x);
    mat[5]  *= static_cast<T>(y);
    mat[9]  *= static_cast<T>(z);
    mat[2]  *= static_cast<T>(x);
    mat[6]  *= static_cast<T>(y);
    nat[10] *= static_cast<T>(z);
    mat[3]  *= static_cast<T>(x);
    mat[7]  *= static_cast<T>(y);
    mat[11] *= static_cast<T>(z);
}

template<typename T>
template<typename U>
vec3<U> mat4<T>::getTranslation() const {
    return vec3<U>(mat[12], mat[13], mat[14]);
}

template<typename T>
mat4<T> mat4<T>::getRotation() const {
    const T zero = static_cast<T>(0);
    const T one = static_cast<T>(1);
    return mat4(mat[0],  mat[1], mat[2],  zero,
                mat[4],  mat[5], mat[6],  zero,
                mat[8],  mat[9], mat[10], zero,
                zero,    zero,   zero,    one);
}

template<typename T>
template<typename U>
void mat4<T>::reflect(U x, U y, U z, U w) {
    reflect(Plane<U>(x, y, z, w));
}

template<typename T>
template<typename U>
void mat4<T>::reflect(const Plane<U> &plane) {
    const U zero = static_cast<U>(0);
    const U one = static_cast<U>(1);

    const vec4<U> &eq = plane.getEquation();

    U x = eq.x;
    U y = eq.y;
    U z = eq.z;
    U w = eq.w;
    U x2 = x * 2.0f;
    U y2 = y * 2.0f;
    U z2 = z * 2.0f;

    set(1 - x * x2, -x * y2, -x * z2, zero,
        -y * x2, 1 - y * y2, -y * z2, zero,
        -z * x2, -z * y2, 1 - z * z2, zero,
        -w * x2, -w * y2, -w * z2,    one);
}

template<typename T>
template<typename U>
void mat4<T>::lookAt(const vec3<U> &eye, const vec3<U> &target, const vec3<U> &up) {
    vec3<U> zAxis(eye - target);
    zAxis.normalize();
    vec3<U> xAxis(Cross(up, zAxis));
    xAxis.normalize();
    vec3<U> yAxis(Cross(zAxis, xAxis));
    yAxis.normalize();

    m[0][0] = static_cast<T>(xAxis.x);
    m[1][0] = static_cast<T>(xAxis.y);
    m[2][0] = static_cast<T>(xAxis.z);
    m[3][0] = static_cast<T>(-xAxis.dot(eye));

    m[0][1] = static_cast<T>(yAxis.x);
    m[1][1] = static_cast<T>(yAxis.y);
    m[2][1] = static_cast<T>(yAxis.z);
    m[3][1] = static_cast<T>(-yAxis.dot(eye));

    m[0][2] = static_cast<T>(zAxis.x);
    m[1][2] = static_cast<T>(zAxis.y);
    m[2][2] = static_cast<T>(zAxis.z);
    m[3][2] = static_cast<T>(-zAxis.dot(eye));

    m[0][3] = static_cast<T>(0);
    m[1][3] = static_cast<T>(0);
    m[2][3] = static_cast<T>(0);
    m[3][3] = static_cast<T>(0);
}

template<typename T>
template<typename U>
void mat4<T>::ortho(U left, U right, U bottom, U top, U zNear, U zFar) {
    zero();

    m[0][0] =  static_cast<T>(2.0f / (right - left));
    m[1][1] =  static_cast<T>(2.0f / (top - bottom));
    m[2][2] = -static_cast<T>(2.0f / (zFar - zNear));
    m[3][3] =  static_cast<T>(1);


    m[3][0] = -static_cast<T>(to_float(right + left) / (right - left));
    m[3][1] = -static_cast<T>(to_float(top + bottom) / (top - bottom));
    m[3][2] = -static_cast<T>(to_float(zFar + zNear) / (zFar - zNear));
}

template<typename T>
template<typename U>
void mat4<T>::perspective(U fovyRad, U aspect, U zNear, U zFar) {
    assert(!IS_ZERO(aspect));
    assert(zFar > zNear);

    F32 tanHalfFovy = std::tan(fovyRad * 0.5f);

    zero();

    m[0][0] =  static_cast<T>(1.0f / (aspect * tanHalfFovy));
    m[1][1] =  static_cast<T>(1.0f / (tanHalfFovy));
    m[2][2] = -static_cast<T>(to_float(zFar + zNear) / (zFar - zNear));
    m[2][3] = -static_cast<T>(1);
    m[3][2] = -static_cast<T>((2.0f * zFar * zNear) / (zFar - zNear));
}

template<typename T>
template<typename U>
void mat4<T>::frustum(U left, U right, U bottom, U top, U nearVal, U farVal) {
    zero();

    m[0][0] = static_cast<T>((2.0f * nearVal) / (right - left));
    m[1][1] = static_cast<T>((2.0f * nearVal) / (top - bottom));
    m[2][0] = static_cast<T>(to_float(right + left) / (right - left));
    m[2][1] = static_cast<T>(to_float(top + bottom) / (top - bottom));
    m[2][2] = -static_cast<T>(to_float(farVal + nearVal) / (farVal - nearVal));
    m[2][3] = static_cast<T>(-1);
    m[3][2] = -static_cast<T>((2.0f * farVal * nearVal) / (farVal - nearVal));
}

template<typename T>
template<typename U>
void mat4<T>::extractMat3(mat3<U> &matrix3) const {
    matrix3.m[0][0] = static_cast<U>(m[0][0]);
    matrix3.m[0][1] = static_cast<U>(m[0][1]);
    matrix3.m[0][2] = static_cast<U>(m[0][2]);
    matrix3.m[1][0] = static_cast<U>(m[1][0]);
    matrix3.m[1][1] = static_cast<U>(m[1][1]);
    matrix3.m[1][2] = static_cast<U>(m[1][2]);
    matrix3.m[2][0] = static_cast<U>(m[2][0]);
    matrix3.m[2][1] = static_cast<U>(m[2][1]);
    matrix3.m[2][2] = static_cast<U>(m[2][2]);
}

template<typename T>
template<typename U>
FORCE_INLINE void mat4<T>::Multiply(const mat4<U>& matrixA, const mat4<U>& matrixB, mat4<U>& ret) {
    // Calling this with matrixA == ret should be safe!
    for (U8 i = 0; i < 4; ++i) {
        ret.setRow(i, matrixB._vec[0] * matrixA._vec[i][0] +
                      matrixB._vec[1] * matrixA._vec[i][1] +
                      matrixB._vec[2] * matrixA._vec[i][2] +
                      matrixB._vec[3] * matrixA._vec[i][3]);
    }
}

// Copyright 2011 The Closure Library Authors. All Rights Reserved.
template<typename T>
FORCE_INLINE void mat4<T>::Inverse(const T* in, T* out) {
    T m00 = in[0],  m10 = in[1],  m20 = in[2],  m30 = in[3];
    T m01 = in[4],  m11 = in[5],  m21 = in[6],  m31 = in[7];
    T m02 = in[8],  m12 = in[9],  m22 = in[10], m32 = in[11];
    T m03 = in[12], m13 = in[13], m23 = in[14], m33 = in[15];

    T a0 = m00 * m11 - m10 * m01;
    T a1 = m00 * m21 - m20 * m01;
    T a2 = m00 * m31 - m30 * m01;
    T a3 = m10 * m21 - m20 * m11;
    T a4 = m10 * m31 - m30 * m11;
    T a5 = m20 * m31 - m30 * m21;
    T b0 = m02 * m13 - m12 * m03;
    T b1 = m02 * m23 - m22 * m03;
    T b2 = m02 * m33 - m32 * m03;
    T b3 = m12 * m23 - m22 * m13;
    T b4 = m12 * m33 - m32 * m13;
    T b5 = m22 * m33 - m32 * m23;

    // should be accurate enough
    F32 idet = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
    if(IS_ZERO(idet)) {
        memcpy(out, in, sizeof(T) * 16);
        return;
    }
    idet = 1.0f / idet;

    out[0]  = ( m11 * b5 - m21 * b4 + m31 * b3) * idet;
    out[1]  = (-m10 * b5 + m20 * b4 - m30 * b3) * idet;
    out[2]  = ( m13 * a5 - m23 * a4 + m33 * a3) * idet;
    out[3]  = (-m12 * a5 + m22 * a4 - m32 * a3) * idet;
    out[4]  = (-m01 * b5 + m21 * b2 - m31 * b1) * idet;
    out[5]  = ( m00 * b5 - m20 * b2 + m30 * b1) * idet;
    out[6]  = (-m03 * a5 + m23 * a2 - m33 * a1) * idet;
    out[7]  = ( m02 * a5 - m22 * a2 + m32 * a1) * idet;
    out[8]  = ( m01 * b4 - m11 * b2 + m31 * b0) * idet;
    out[9]  = (-m00 * b4 + m10 * b2 - m30 * b0) * idet;
    out[10] = ( m03 * a4 - m13 * a2 + m33 * a0) * idet;
    out[11] = (-m02 * a4 + m12 * a2 - m32 * a0) * idet;
    out[12] = (-m01 * b3 + m11 * b1 - m21 * b0) * idet;
    out[13] = ( m00 * b3 - m10 * b1 + m20 * b0) * idet;
    out[14] = (-m03 * a3 + m13 * a1 - m23 * a0) * idet;
    out[15] = ( m02 * a3 - m12 * a1 + m22 * a0) * idet;
}

}; //namespace Divide
#endif //_MATH_MATRICES_INL_

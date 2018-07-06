/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _QUATERNION_H_
#define _QUATERNION_H_

/*
http://gpwiki.org/index.php/OpenGL:Tutorials:Using_Quaternions_to_represent_rotation
Quaternion class based on code from " OpenGL:Tutorials:Using Quaternions to represent rotation "
*/
#include "core.h"

template<class T>
class Quaternion
{
public:
    Quaternion(T x, T y, T z, T w) : _x(x), _y(y), _z(z), _w(w),_dirty(true){}
    Quaternion() : _x(0), _y(0), _z(0), _w(1),_dirty(true) {}
    Quaternion(const mat3<T>& rotationMatrix) : _dirty(true){fromMatrix(rotationMatrix);}
    Quaternion(const vec3<T>& axis, T angle) : _dirty(true) {fromAxisAngle(axis, angle);}
    Quaternion(T pitch, T yaw, T roll,bool inDegrees = true) : _dirty(true) {fromEuler(pitch,yaw,roll,inDegrees);}
    Quaternion(const Quaternion& q) : _dirty(true) { set(q); }

    inline T dot(const Quaternion& rq)  const { return _w*rq._w+_x*rq._x+_y*rq._y+_z*rq._z;  }
    inline T magnitude()                const { return sqrtf(magnituteSq()); }
    inline T magnituteSq()              const { return (_w * _w + _x * _x + _y * _y + _z * _z); }

    inline bool compare(const Quaternion& rq, F32 tolerance = 1e-3f) const {
        T fCos = dot(rq);
        T angle = (T)acos((D32)fCos);
        T angleRad = RADIANS(angle);
        F32 toleranceRad = RADIANS(tolerance);

        return (abs(angleRad) <= toleranceRad) || FLOAT_COMPARE_TOLERANCE(angleRad, M_PI, toleranceRad);
    }

    inline void set(T x, T y, T z, T w) {_x = x; _y = y; _z = z; _w = w; _dirty = true;}

    inline void set(const Quaternion& q) {_x = q._x; _y = q._y; _z = q._z; _w = q._w; _dirty = true; } 

    //! normalising a quaternion works similar to a vector. This method will not do anything
    //! if the quaternion is close enough to being unit-length. define EPSILON as something
    //! small like 0.00001f to get accurate results
    inline void normalize() {
        // Don't normalize if we don't have to
        T mag2 = (_w * _w + _x * _x + _y * _y + _z * _z);
        if (  mag2!=0.f && (fabs(mag2 - 1.0f) > EPSILON)) {
            T mag = square_root_tpl(mag2);
            _w /= mag;
            _x /= mag;
            _y /= mag;
            _z /= mag;
            _dirty = true;
        }
    }

    inline Quaternion inverse() const {
        T invMag = 1.0f / magnitude();
        return getConjugate() * invMag;
    }

    //! We need to get the inverse of a quaternion to properly apply a quaternion-rotation to a vector
    //! The conjugate of a quaternion is the same as the inverse, as long as the quaternion is unit-length
    inline Quaternion getConjugate() const { return Quaternion<T>(-_x, -_y, -_z, _w);}

    //! Multiplying q1 with q2 applies the rotation q2 to q1
    //! the constructor takes its arguments as (x, y, z, w)
    inline Quaternion operator* (const Quaternion& rq) const {
        return Quaternion<T>(_w * rq._x + _x * rq._w + _y * rq._z - _z * rq._y,
                             _w * rq._y + _y * rq._w + _z * rq._x - _x * rq._z,
                             _w * rq._z + _z * rq._w + _x * rq._y - _y * rq._x,
                             _w * rq._w - _x * rq._x - _y * rq._y - _z * rq._z);
    }

    //! Multiply so that rotations are applied in a left to right order.
    inline Quaternion& operator*=(const Quaternion& rq){
        Quaternion tmp((_w * rq._w) - (_x * rq._x) - (_y * rq._y) - (_z * rq._z),
                       (_w * rq._x) + (_x * rq._w) - (_y * rq._z) + (_z * rq._y),
                       (_w * rq._y) + (_x * rq._z) + (_y * rq._w) - (_z * rq._x),
                       (_w * rq._z) - (_x * rq._y) + (_y * rq._x) + (_z * rq._w));

        *this = tmp;
        _dirty = true;
        return *this;
    }

    //! Multiplying a quaternion q with a vector v applies the q-rotation to v
    vec3<T>  operator* (const vec3<T>& vec) const {
        // nVidia SDK implementation
        vec3<T> uv, uuv;
        vec3<T> qvec(_x, _y, _z);
        uv.cross(qvec,vec);
        uuv.cross(qvec,uv);
        uv *= (2.0f * _w);
        uuv *= 2.0f;

        return vec + uv + uuv;
    }

    inline bool operator==(const Quaternion& rq) const { return compare(rq); }
    inline bool operator!=(const Quaternion& rq) const { return !(*this == rq); }

    inline Quaternion& operator+=(const Quaternion& rq) {
        _w += rq._w;
        _x += rq._x;
        _y += rq._y;
        _z += rq._z;
        _dirty = true;
        return *this;
    }

    inline Quaternion& operator-=(const Quaternion& rq) {
        _w -= rq._w;
        _x -= rq._x;
        _y -= rq._y;
        _z -= rq._z;
        _dirty = true;
        return *this;
    }

    inline Quaternion& operator*=(T scalar) {
        _w *= scalar;
        _x *= scalar;
        _y *= scalar;
        _z *= scalar;
        _dirty = true;
        return *this;
    }

    inline Quaternion& operator/=(T scalar) {
        _w /= scalar, _x /= scalar, _y /= scalar, _z /= scalar;
        _dirty = true;
        return *this;
    }

    inline Quaternion operator+(const Quaternion& rq) const {
        Quaternion tmp(*this);
        tmp += rq;
        return tmp;
    }

    inline Quaternion operator-(const Quaternion& rq) const {
        Quaternion tmp(*this);
        tmp -= rq;
        return tmp;
    }

    inline Quaternion operator*(T scalar) const {
        Quaternion tmp(*this);
        tmp *= scalar;
        return tmp;
    }

    inline Quaternion operator/(T scalar) const {
        Quaternion tmp(*this);
        tmp /= scalar;
        return tmp;
    }

    inline void slerp(const Quaternion& q, F32 t) {	slerp(this, q1, t); }

    void slerp(const Quaternion& q0,const Quaternion& q1,F32 t) {
        F32 k0,k1,cosomega = q0._x * q1._x + q0._y * q1._y + q0._z * q1._z + q0._w * q1._w;
        Quaternion q;
        if(cosomega < 0.0) {
            cosomega = -cosomega;
            q._x = -q1._x;
            q._y = -q1._y;
            q._z = -q1._z;
            q._w = -q1._w;
        } else {
            q._x = q1._x;
            q._y = q1._y;
            q._z = q1._z;
            q._w = q1._w;
        }
        if(1.0 - cosomega > 1e-6) {
            F32 omega = (F32)acos(cosomega);
            F32 sinomega = (F32)sin(omega);
            k0 = (F32)sin((1.0f - t) * omega) / sinomega;
            k1 = (F32)sin(t * omega) / sinomega;
        } else {
            k0 = 1.0f - t;
            k1 = t;
        }
        this->_x = q0._x * k0 + q._x * k1;
        this->_y = q0._y * k0 + q._y * k1;
        this->_z = q0._z * k0 + q._z * k1;
        this->_w = q0._w * k0 + q._w * k1;

        _dirty = true;
    }

    //! Convert from Axis Angle
    void fromAxisAngle(const vec3<T>& v, T angle){
        _dirty = true;
        T sinAngle;
        angle = RADIANS(angle);
        angle *= 0.5f;
        vec3<T> vn(v);
        vn.normalize();

        sinAngle = sin(angle);

        _x = (vn.x * sinAngle);
        _y = (vn.y * sinAngle);
        _z = (vn.z * sinAngle);
        _w = cos(angle);
    }

    inline void fromEuler(const vec3<T>& v, bool inDegrees = true) {
        fromEuler(v.pitch,v.yaw,v.roll,inDegrees);
    }

    //! Convert from Euler Angles
    void  fromEuler(T pitch, T yaw, T roll, bool inDegrees = true) {
        _dirty = true;

        T attitude = pitch;
        T heading = yaw;
        T bank = roll;

        if(inDegrees){
            attitude = RADIANS(attitude);
            heading  = RADIANS(heading);
            bank     = RADIANS(bank);
        }

        D32 c1 = cos(heading  * 0.5);
        D32 s1 = sin(heading  * 0.5);
        D32 c2 = cos(attitude * 0.5);
        D32 s2 = sin(attitude * 0.5);
        D32 c3 = cos(bank * 0.5);
        D32 s3 = sin(bank * 0.5);

        D32 c1c2 = c1*c2;
        D32 s1s2 = s1*s2;

        this->_w = (T)(c1c2 * c3 - s1s2 * s3);
        this->_x = (T)(c1c2 * s3 + s1s2 * c3);
        this->_y = (T)(s1 * c2 * c3 + c1 * s2 * s3);
        this->_z = (T)(c1 * s2 * c3 - s1 * c2 * s3);

        //normalize(); this method does produce a normalized quaternion
    }

    //a la Ogre3D
    void fromMatrix(const mat3<T>& rotationMatrix) {
        _dirty = true;

        // Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
        // article "Quaternion Calculus and Fast Animation".

        T fTrace = rotationMatrix.m[0][0]+rotationMatrix.m[1][1]+rotationMatrix.m[2][2];
        T fRoot;

        if ( fTrace > 0.0 ){
            // |w| > 1/2, may as well choose w > 1/2
            fRoot = (T)sqrt((F32)fTrace + 1.0f);  // 2w
            _w = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;  // 1/(4w)
            _x = (rotationMatrix.m[2][1]-rotationMatrix.m[1][2])*fRoot;
            _y = (rotationMatrix.m[0][2]-rotationMatrix.m[2][0])*fRoot;
            _z = (rotationMatrix.m[1][0]-rotationMatrix.m[0][1])*fRoot;
        }else{
            // |w| <= 1/2
            static size_t s_iNext[3] = { 1, 2, 0 };
            size_t i = 0;
            if ( rotationMatrix.m[1][1] > rotationMatrix.m[0][0] )
                i = 1;
            if ( rotationMatrix.m[2][2] > rotationMatrix.m[i][i] )
                i = 2;
            size_t j = s_iNext[i];
            size_t k = s_iNext[j];

            fRoot = (T)sqrt((F32)(rotationMatrix.m[i][i]-rotationMatrix.m[j][j]-rotationMatrix.m[k][k] + 1.0f));
            T* apkQuat[3] = { &_x, &_y, &_z };
            *apkQuat[i] = 0.5f*fRoot;
            fRoot = 0.5f/fRoot;
            _w = (rotationMatrix.m[k][j]-rotationMatrix.m[j][k])*fRoot;
            *apkQuat[j] = (rotationMatrix.m[j][i]+rotationMatrix.m[i][j])*fRoot;
            *apkQuat[k] = (rotationMatrix.m[k][i]+rotationMatrix.m[i][k])*fRoot;
        }
    }

    //! Convert to Matrix
    const mat4<T>& getMatrix(){
        if(_dirty) {
            T xx      = _x * _x;
            T xy      = _x * _y;
            T xz      = _x * _z;
            T xw      = _x * _w;
            T yy      = _y * _y;
            T yz      = _y * _z;
            T yw      = _y * _w;
            T zz      = _z * _z;
            T zw      = _z * _w;
            _mat.mat[0]  = 1 - 2 * ( yy + zz );
            _mat.mat[1]  =     2 * ( xy - zw );
            _mat.mat[2]  =     2 * ( xz + yw );
            _mat.mat[4]  =     2 * ( xy + zw );
            _mat.mat[5]  = 1 - 2 * ( xx + zz );
            _mat.mat[6]  =     2 * ( yz - xw );
            _mat.mat[8]  =     2 * ( xz - yw );
            _mat.mat[9]  =     2 * ( yz + xw );
            _mat.mat[10] = 1 - 2 * ( xx + yy );
            _dirty = false;
        }
        return _mat;
    }

    //! Convert to Axis/Angles
    void  getAxisAngle(vec3<T> *axis, T *angle,bool inDegrees) const {
        T scale = square_root_tpl(_x * _x + _y * _y + _z * _z);
        axis->x = _x / scale;
        axis->y = _y / scale;
        axis->z = _z / scale;
        if(inDegrees) *angle = DEGREES(acos(_w) * 2.0f);
        else		  *angle = acos(_w) * 2.0f;
    }

    void getEuler(vec3<T> *euler, bool toDegrees = false) const {
        T heading = 0, attitude = 0, bank = 0;

        T sqx = _x * _x;
        T sqy = _y * _y;
        T sqz = _z * _z;
        T sqw = _w * _w;
        T test = _x * _y + _z * _w;
        T unit = sqx + sqy + sqz + sqw; // if normalised is one, otherwise is correction factor

        if(test > (0.5f-EPSILON) * unit) { // singularity at north pole
            heading  = 2 * atan2(_x , _w);
            attitude = M_PIDIV2;
            bank     = 0;
        }else if (test < -(0.5f - EPSILON) * unit) { // singularity at south pole
            heading  = -2 * atan2(_x , _w);
            attitude = -M_PIDIV2;
            bank     = 0;
        }else{
            T x2   = 2 * _x;
            T y2   = 2 * _y;

            heading  = atan2(y2 * _w - x2 * _z , sqx - sqy - sqz + sqw);
            attitude = asin(2 * test / unit);
            bank     = atan2(x2 * _w - y2 * _z ,-sqx + sqy - sqz + sqw);
        }
        //Convert back from Z = pitch to Z = roll
        if(toDegrees){
            euler->yaw   = DEGREES(heading);
            euler->pitch = DEGREES(bank);
            euler->roll  = DEGREES(attitude);
        }else{
            euler->yaw   = heading;
            euler->pitch = bank;
            euler->roll  = attitude;
        }
    }

    inline       void    identity()       { _w = 1, _x = _y = _z = 0; _dirty = true;}
    inline const T&      getX()     const {return _x;}
    inline const T&      getY()     const {return _y;}
    inline const T&      getZ()     const {return _z;}
    inline const T&      getW()     const {return _w;}
    inline       vec4<T> asVec4()   const {return vec4<T>(_x,_y,_z,_w);}

private:
    T _x,_y,_z,_w;
    mat4<T> _mat;
    bool _dirty;
};
#endif
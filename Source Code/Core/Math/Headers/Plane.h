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

// This file is based on material originally from:
// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//This code is HEAVILY inspired by Ogre3D and Torque3D
#ifndef _CORE_MATH_PLANE_H_
#define _CORE_MATH_PLANE_H_

#include "MathVectors.h"
#include "Utility/Headers/Vector.h"
///This class defines a 3D plane defined as Ax + By + Cz + D = 0
///This class is equivalent to a vector, the plane's normal,
///whose x, y and z components equate to the coefficients A, B and Crespectively
///and a constant (D) which is the distance along the normal you have to go to move the plane back to the origin.
template<class T>
class Plane {
public:
    /** From Ogre3D: The "positive side" of the plane is the half space to which the
        plane normal points. The "negative side" is the other half
        space. The flag "no side" indicates the plane itself.
    */
    enum Side {
        NO_SIDE,
        POSITIVE_SIDE,
        NEGATIVE_SIDE,
        BOTH_SIDE
    };

    Plane() : _distance(0), _active(true) {}
    Plane(const Plane& rhs) : _normal(rhs._normal), _distance(rhs._distance), _active(true) {}
    ///distance is stored as the negative of the specified value
    Plane(const vec3<T>& normal, T distance) : _normal(normal), _distance(-distance), _active(true) {}
    ///distance is stored as the negative of the specified value
    Plane(T a, T b, T c, T distance) : _normal(a, b, c), _distance(distance), _active(true){}
    Plane(const vec3<T>& normal, const vec3<T>& point) : _active(true)
    {
        redefine(normal, point);
    }
    Plane (const vec3<T>& point0, const vec3<T>& point1, const vec3<T>& point2) : _active(true)
    {
        redefine(point0, point1, point2);
    }

    inline T getDistance(const vec3<T>& point) const{	return _normal.dot(point) + _distance; }

    inline void set(const vec3<T>& normal, T distance) {_normal = normal; _distance = -distance;}

    inline void redefine(const vec3<T>& point0, const vec3<T>& point1, const vec3<T>& point2) {
        vec3<T> edge1 = point1 - point0;
        vec3<T> edge2 = point2 - point0;
        _normal = edge1.cross(edge2);
        _normal.normalize();
        _distance = -_normal.dot(point0);
    }

    inline void redefine(const vec3<T>& normal, const vec3<T>& point) {
        _normal = normal;
        _distance = -_normal.dot(point);
    }
    inline vec4<T> getEquation() const {return vec4<T>(_normal,_distance);}

    /// active plane state. used by rendering API's when the plane is considered a clipplane
    inline bool active()           const {return _active;}
    inline void active(bool state)       {_active = state;}
    /// Comparison operator
    bool operator==(const Plane& rhs) const { return (rhs._distance == _distance && rhs._normal == _normal); }
    bool operator!=(const Plane& rhs) const { return (rhs._distance != _distance || rhs._normal != _normal); }

    T normalize() {
        T length = _normal.length();
        if ( length > F32(0.0f) ) {
            T invLength = 1.0f / length;
            _normal *= invLength;
            _distance *= invLength;
        }
        return length;
    }

protected:
    vec3<T> _normal;
    T _distance;
    bool _active;
};

typedef vectorImpl<Plane<F32> > PlaneList;

#endif
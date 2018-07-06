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

// This file is based on material originally from:
// Geometric Tools, LLC
// Copyright (c) 1998-2010
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// This code is HEAVILY inspired by Ogre3D and Torque3D
#ifndef _CORE_MATH_PLANE_H_
#define _CORE_MATH_PLANE_H_

#include "MathUtil.h"

namespace Divide {

/// This class defines a 3D plane defined as Ax + By + Cz + D = 0
/// This class is equivalent to a vector, the plane's normal,
/// whose x, y and z components equate to the coefficients A, B and C
/// respectively
/// and a constant (D) which is the distance along the normal you have to go to
/// move the plane back to the origin.
template <typename T>
class Plane {
   public:
    /** From Ogre3D: The "positive side" of the plane is the half space to which
       the
        plane normal points. The "negative side" is the other half
        space. The flag "no side" indicates the plane itself.
        */
    enum class Side : I32 {
        NO_SIDE,
        POSITIVE_SIDE,
        NEGATIVE_SIDE
    };

    Plane() : _distance(0)
    {
    }

    Plane(const Plane& rhs)
        : _normal(rhs._normal),
          _distance(rhs._distance)
    {
    }

    /// distance is stored as the negative of the specified value
    Plane(const vec3<T>& normal, T distance)
        : _normal(normal), _distance(distance)
    {
    }

    /// distance is stored as the negative of the specified value
    Plane(T a, T b, T c, T distance) : Plane(vec3<T>(a, b, c), distance)
    {
    }

    Plane(const vec4<T>& plane) : Plane(plane.xyz(), plane.w)
    {
    }

    Plane(const vec3<T>& normal, const vec3<T>& point)
    {
        redefine(normal, point);
    }

    Plane(const vec3<T>& point0, const vec3<T>& point1, const vec3<T>& point2)
    {
        redefine(point0, point1, point2);
    }

    Plane& operator=(const Plane& other) {
        _normal.set(other._normal);
        _distance = other._distance;

        return *this;
    }

    inline Side classifyPoint(const vec3<F32>& point) const {
        F32 result = signedDistanceToPoint(point);
        return (result > 0 ? Side::POSITIVE_SIDE
                           : (result < 0 ? Side::NEGATIVE_SIDE : Side::NO_SIDE));
    }

    inline T signedDistanceToPoint(const vec3<T>& point) const {
        return _normal.dot(point) + _distance;
    }

    vec3<T> closestPointOnPlaneToPoint(const vec3<T>& point) const  {
        return (point - signedDistanceToPoint(point)) * _normal;
    }

    inline T& getDistance() { return _distance; }
    inline T  getDistance() const { return _distance; }

    inline void set(const vec4<T>& equation) {
        set(equation.xyz(), equation.w);
    }

    inline void set(const vec3<T>& normal, T distance) {
        _normal = normal;
        _distance = distance;
    }

    inline void set(T a, T b, T c, T distance) {
        set(vec3<T>(a, b, c), distance);
    }

    inline void redefine(const vec3<T>& point0, const vec3<T>& point1,
                         const vec3<T>& point2) {
        vec3<T> edge1 = point1 - point0;
        vec3<T> edge2 = point2 - point0;
        _normal = edge1.cross(edge2);
        _normal.normalize();
        _distance = _normal.dot(point0);
    }

    inline void redefine(const vec3<T>& normal, const vec3<T>& point) {
        _normal = normal;
        _distance = _normal.dot(point);
    }

    inline const vec4<T>& getEquation() const { return _equation; }

    inline vec3<T>& getNormal() { return _normal; }
    inline const vec3<T>& getNormal() const { return _normal; }

    /// Comparison operator
    bool operator==(const Plane& rhs) const {
        return COMPARE(rhs._distance, _distance) && rhs._normal == _normal;
    }

    bool operator!=(const Plane& rhs) const {
        return !COMPARE(rhs._distance, _distance) || rhs._normal != _normal;
    }

    bool compare(const Plane& rhs, F32 epsilon) {
        return COMPARE_TOLERANCE(rhs._distance, _distance, epsilon) && rhs._normal.compare(_normal, epsilon);
    }

    T normalize() {
        T length = _normal.length();
        if (length > 0.0f) {
            T invLength = 1.0f / length;
            _normal *= invLength;
            _distance *= invLength;
        }
        
        return length;
    }

   protected:
       union {
           struct {
               vec3<T> _normal;
               T _distance;
           };

           vec4<T> _equation;
       };
};

template<size_t N>
using PlaneList = std::array<Plane<F32>, N>;

typedef vectorImpl<Plane<F32>> PlaneDynamicList;
};  // namespace Divide

#endif
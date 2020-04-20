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

#pragma once
#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_

#include "BoundingBox.h"

#include "Core/Math/Headers/Ray.h"

namespace Divide {
namespace Attorney {
    class BoundingSphereEditor;
};

class PropertyWindow;
class BoundingSphere {
    friend class Attorney::BoundingSphereEditor;

   public:
    BoundingSphere();
    BoundingSphere(const vec3<F32>& center, F32 radius) noexcept;
    BoundingSphere(const vectorEASTL<vec3<F32>>& points) noexcept;
    BoundingSphere(const std::array<vec3<F32>, 8>& points) noexcept;

    BoundingSphere(const BoundingSphere& s) noexcept;
    void operator=(const BoundingSphere& s) noexcept;

    void fromBoundingBox(const BoundingBox& bBox);
    void fromBoundingSphere(const BoundingSphere& bSphere);
    bool containsPoint(const vec3<F32>& point) const noexcept;
    bool containsBoundingBox(const BoundingBox& AABB) const noexcept;

    // https://code.google.com/p/qe3e/source/browse/trunk/src/BoundingSphere.h?r=28
    void add(const BoundingSphere& bSphere);
    void add(const vec3<F32>& point);
    void addRadius(const BoundingSphere& bSphere);
    void addRadius(const vec3<F32>& point);

    void createFromPoints(const vectorEASTL<vec3<F32>>& points) noexcept;
    void createFromPoints(const std::array<vec3<F32>, 8>& points) noexcept;

    void setRadius(F32 radius) noexcept;
    void setCenter(const vec3<F32>& center) noexcept;

    const vec3<F32>& getCenter() const noexcept;
    F32 getRadius() const noexcept;
    F32 getDiameter() const noexcept;

    F32 getDistanceFromPoint(const vec3<F32>& point) const;

    void reset();
    vec4<F32> asVec4() const;

    bool collision(const BoundingSphere& sphere2) const noexcept;

   private:
    bool _visibility, _dirty;
    vec3<F32> _center;
    F32 _radius;
    //mutable SharedMutex _lock;
};

namespace Attorney {
    class BoundingSphereEditor {
        private:
        static F32* center(BoundingSphere& bs) noexcept {
            return bs._center._v;
        }
        static F32& radius(BoundingSphere& bs) noexcept {
            return bs._radius;
        }
        friend class Divide::PropertyWindow;
    };
}; //namespace Attorney

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_

#include "BoundingSphere.inl"
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

#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_

#include "BoundingBox.h"

#include "Core/Math/Headers/Ray.h"
#include "Platform/Threading/Headers/SharedMutex.h"

namespace Divide {

class BoundingSphere {
   public:
    BoundingSphere();
    BoundingSphere(const vec3<F32>& center, F32 radius);
    BoundingSphere(vectorImpl<vec3<F32>>& points);

    BoundingSphere(const BoundingSphere& s);
    void operator=(const BoundingSphere& s);

    void fromBoundingBox(const BoundingBox& bBox);

    // https://code.google.com/p/qe3e/source/browse/trunk/src/BoundingSphere.h?r=28
    void add(const BoundingSphere& bSphere);
    void add(const vec3<F32>& point);
    void addRadius(const BoundingSphere& bSphere);
    void addRadius(const vec3<F32>& point);

    void CreateFromPoints(vectorImpl<vec3<F32>>& points);

    void setRadius(F32 radius);
    void setCenter(const vec3<F32>& center);

    const vec3<F32>& getCenter() const;
    F32 getRadius() const;
    F32 getDiameter() const;

   private:
    bool _computed, _visibility, _dirty;
    vec3<F32> _center;
    F32 _radius;
    mutable SharedLock _lock;
};

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGSPHERE_H_

#include "BoundingSphere.inl"
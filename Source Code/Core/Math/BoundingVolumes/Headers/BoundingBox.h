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

#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_

#include "Core/Math/Headers/Ray.h"
#include "Utility/Headers/GUIDWrapper.h"

namespace Divide {

class BoundingSphere;
class BoundingBox : public GUIDWrapper {
   public:
    BoundingBox();
    BoundingBox(const vec3<F32>& min, const vec3<F32>& max);
    BoundingBox(const vectorImpl<vec3<F32>>& points);
    BoundingBox(F32 minX, F32 minY, F32 minZ, F32 maxX, F32 maxY, F32 maxZ);
    ~BoundingBox();

    BoundingBox(const BoundingBox& b);
    void operator=(const BoundingBox& b);

    bool containsPoint(const vec3<F32>& point) const;
    bool containsBox(const BoundingBox& AABB2) const;
    bool containsSphere(const BoundingSphere& bSphere) const;

    bool collision(const BoundingBox& AABB2) const;
    bool collision(const BoundingSphere& bSphere) const;

    bool compare(const BoundingBox& bb) const;
    bool operator==(const BoundingBox& B) const;
    bool operator!=(const BoundingBox& B) const;

    /// Optimized method
    bool intersect(const Ray& r, F32 t0, F32 t1) const;

    void createFromPoints(const vectorImpl<vec3<F32>>& points);
    void createFromSphere(const vec3<F32>& center, F32 radius);

    void add(const vec3<F32>& v);
    void add(const BoundingBox& bb);

    void translate(const vec3<F32>& v);

    void multiply(F32 factor);
    void multiply(const vec3<F32>& v);
    void multiplyMax(const vec3<F32>& v);
    void multiplyMin(const vec3<F32>& v);

    void transform(const BoundingBox& initialBoundingBox,
                   const mat4<F32>& mat);

    void transform(const mat4<F32>& mat);

    const vec3<F32>& getMin() const;
    const vec3<F32>& getMax() const;

    vec3<F32> getCenter() const;
    vec3<F32> getExtent() const;
    vec3<F32> getHalfExtent() const;

    F32 getWidth() const;
    F32 getHeight() const;
    F32 getDepth() const;

    void set(const BoundingBox& bb);
    void set(const vec3<F32>& min, const vec3<F32>& max);
    void setMin(const vec3<F32>& min);
    void setMax(const vec3<F32>& max);

    void set(F32 min, F32 max);
    void set(F32 minX, F32 minY, F32 minZ, F32 maxX, F32 maxY, F32 maxZ);
    void setMin(F32 min);
    void setMin(F32 minX, F32 minY, F32 minZ);
    void setMax(F32 max);
    void setMax(F32 maxX, F32 maxY, F32 maxZ);

    void reset();

    std::array<vec3<F32>, 8> getPoints() const;

    F32 nearestDistanceFromPointSquared(const vec3<F32>& pos) const;
    F32 nearestDistanceFromPoint(const vec3<F32>& pos) const;

    inline vec3<F32> getPVertex(const vec3<F32>& normal) const;
    inline vec3<F32> getNVertex(const vec3<F32>& normal) const;
   private:
    vec3<F32> _min, _max;
};

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_

#include "BoundingBox.inl"

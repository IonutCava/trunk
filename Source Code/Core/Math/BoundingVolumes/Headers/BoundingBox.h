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

#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_

#include "Core/Math/Headers/Ray.h"
#include "Utility/Headers/GUIDWrapper.h"
#include <algorithm>

namespace Divide {

class BoundingBox : public GUIDWrapper {
   public:
    BoundingBox();
    BoundingBox(const vec3<F32>& min, const vec3<F32>& max);
    BoundingBox(vectorImpl<vec3<F32>>& points);
    ~BoundingBox();

    BoundingBox(const BoundingBox& b);
    void operator=(const BoundingBox& b);

    bool ContainsPoint(const vec3<F32>& point) const;

    bool Collision(const BoundingBox& AABB2) const;

    bool Compare(const BoundingBox& bb) const;
    bool operator==(const BoundingBox& B) const;
    bool operator!=(const BoundingBox& B) const;

    /// Optimized method
    bool Intersect(const Ray& r, F32 t0, F32 t1) const;

    void CreateFromPoints(vectorImpl<vec3<F32>>& points);

    void Add(const vec3<F32>& v);
    void Add(const BoundingBox& bb);

    void Translate(const vec3<F32>& v);

    void Multiply(F32 factor);
    void Multiply(const vec3<F32>& v);
    void MultiplyMax(const vec3<F32>& v);
    void MultiplyMin(const vec3<F32>& v);

    bool Transform(const BoundingBox& initialBoundingBox, const mat4<F32>& mat,
                   bool force = false);

    void setComputed(bool state);
    bool isComputed() const;

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

    void reset();

    const vec3<F32>* getPoints() const;

    F32 nearestDistanceFromPointSquared(const vec3<F32>& pos) const;
    F32 nearestDistanceFromPoint(const vec3<F32>& pos) const;

   protected:
    void ComputePoints() const;

   private:
    bool _computed;
    vec3<F32> _min, _max;
    mat4<F32> _oldMatrix;

    // This is is very limited in scope so mutable should be ok
    mutable bool _pointsDirty;
    mutable vec3<F32> _points[8];
    mutable vec3<F32> _cacheVector;
};

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_H_

#include "BoundingBox.inl"

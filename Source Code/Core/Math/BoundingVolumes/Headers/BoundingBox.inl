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

#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_

namespace Divide {

inline bool BoundingBox::containsPoint(const vec3<F32>& point) const noexcept {
    // const SharedLock<SharedMutex> r_lock(_lock);
    return (IS_GEQUAL(point.x, _min.x) && IS_GEQUAL(point.y, _min.y) && IS_GEQUAL(point.z, _min.z) &&
            IS_LEQUAL(point.x, _max.x) && IS_LEQUAL(point.y, _max.y) && IS_LEQUAL(point.z, _max.z));
}

inline bool BoundingBox::compare(const BoundingBox& bb) const  noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _min == bb._min &&
           _max == bb._max;
}

inline bool BoundingBox::operator==(const BoundingBox& B) const noexcept {
    return compare(B);
}

inline bool BoundingBox::operator!=(const BoundingBox& B) const noexcept {
    return !compare(B);
}

inline void BoundingBox::createFromPoints(const vectorSTD<vec3<F32>>& points) noexcept {
    for (const vec3<F32>& p : points) {
        add(p);
    }
}

inline void BoundingBox::createFromPoints(const std::array<vec3<F32>, 8>& points) noexcept {
    for (const vec3<F32>& p : points) {
        add(p);
    }
}

inline void BoundingBox::createFromSphere(const vec3<F32>& center, F32 radius) noexcept {
    _max.set(center + radius);
    _min.set(center - radius);
}

inline void BoundingBox::add(const vec3<F32>& v) noexcept {
    // Lock w_lock(_lock);
    //Max
    if (v.x > _max.x) {
        _max.x = v.x;
    }
    if (v.y > _max.y) {
        _max.y = v.y;
    }
    if (v.z > _max.z) {
        _max.z = v.z;
    }

    //Min
    if (v.x < _min.x) {
        _min.x = v.x;
    }
    if (v.y < _min.y) {
        _min.y = v.y;
    }
    if (v.z < _min.z) {
        _min.z = v.z;
    }
};

inline void BoundingBox::add(const BoundingBox& bb) noexcept {
    // Lock w_lock(_lock);
    _max.set(std::max(bb._max.x, _max.x),
             std::max(bb._max.y, _max.y),
             std::max(bb._max.z, _max.z));

    _min.set(std::min(bb._min.x, _min.x),
             std::min(bb._min.y, _min.y),
             std::min(bb._min.z, _min.z));
}

inline void BoundingBox::translate(const vec3<F32>& v) noexcept {
    // Lock w_lock(_lock);
    _min += v;
    _max += v;
}

inline void BoundingBox::multiply(F32 factor) noexcept {
    // Lock w_lock(_lock);
    _min *= factor;
    _max *= factor;
}

inline void BoundingBox::multiply(const vec3<F32>& v) noexcept {
    // Lock w_lock(_lock);
    _min.x *= v.x;
    _min.y *= v.y;
    _min.z *= v.z;
    _max.x *= v.x;
    _max.y *= v.y;
    _max.z *= v.z;
}

inline void BoundingBox::multiplyMax(const vec3<F32>& v) noexcept {
    // Lock w_lock(_lock);
    _max.x *= v.x;
    _max.y *= v.y;
    _max.z *= v.z;
}

inline void BoundingBox::multiplyMin(const vec3<F32>& v) noexcept {
    // Lock w_lock(_lock);
    _min.x *= v.x;
    _min.y *= v.y;
    _min.z *= v.z;
}

inline const vec3<F32>& BoundingBox::getMin() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _min;
}

inline const vec3<F32>& BoundingBox::getMax() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _max;
}

inline vec3<F32> BoundingBox::getCenter() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return (_max + _min) * 0.5f;
}

inline vec3<F32> BoundingBox::getExtent() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _max - _min;
}

inline vec3<F32> BoundingBox::getHalfExtent() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return (_max - _min) * 0.5f;
}

inline F32 BoundingBox::getWidth() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _max.x - _min.x;
}

inline F32 BoundingBox::getHeight() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _max.y - _min.y;
}

inline F32 BoundingBox::getDepth() const noexcept {
    /*SharedLock<SharedMutex> r_lock(_lock);*/
    return _max.z - _min.z;
}

inline void BoundingBox::setMin(const vec3<F32>& min) noexcept {
    setMin(min.x, min.y, min.z);
}

inline void BoundingBox::setMax(const vec3<F32>& max) noexcept {
    setMax(max.x, max.y, max.z);
}

inline void BoundingBox::set(const BoundingBox& bb) noexcept {
    set(bb._min, bb._max); 
}

inline void BoundingBox::set(F32 min, F32 max) noexcept {
    _min.set(min);
    _max.set(max);
}

inline void BoundingBox::set(F32 minX, F32 minY, F32 minZ, F32 maxX, F32 maxY, F32 maxZ) noexcept {
    _min.set(minX, minY, minZ);
    _max.set(maxX, maxY, maxZ);
}

inline void BoundingBox::setMin(F32 min) noexcept {
    _min.set(min);
}

inline void BoundingBox::setMin(F32 minX, F32 minY, F32 minZ) noexcept {
    _min.set(minX, minY, minZ);
}

inline void BoundingBox::setMax(F32 max) noexcept {
    _max.set(max);
}

inline void BoundingBox::setMax(F32 maxX, F32 maxY, F32 maxZ) noexcept {
    _max.set(maxX, maxY, maxZ);
}

inline void BoundingBox::set(const vec3<F32>& min, const vec3<F32>& max) noexcept {
    /*Lock w_lock(_lock);*/
    _min = min;
    _max = max;
}

inline void BoundingBox::reset() noexcept {
    /*Lock w_lock(_lock);*/
    _min.set( std::numeric_limits<F32>::max());
    _max.set(-std::numeric_limits<F32>::max());
}

inline std::array<vec3<F32>, 8> BoundingBox::getPoints() const noexcept {
    return std::array<vec3<F32>, 8>
    {
        vec3<F32>{_min.x, _min.y, _min.z},
        vec3<F32>{_min.x, _min.y, _max.z},
        vec3<F32>{_min.x, _max.y, _min.z},
        vec3<F32>{_min.x, _max.y, _max.z},
        vec3<F32>{_max.x, _min.y, _min.z},
        vec3<F32>{_max.x, _min.y, _max.z},
        vec3<F32>{_max.x, _max.y, _min.z},
        vec3<F32>{_max.x, _max.y, _max.z}
    };
}

inline F32 BoundingBox::nearestDistanceFromPoint(const vec3<F32>& pos) const {
    return Divide::Sqrt(nearestDistanceFromPointSquared(pos));
}

inline vec3<F32> BoundingBox::getPVertex(const vec3<F32>& normal) const {
    return vec3<F32>(normal.x >= 0.0f ? _max.x : _min.x,
                     normal.y >= 0.0f ? _max.y : _min.y,
                     normal.z >= 0.0f ? _max.z : _min.z);
}

inline vec3<F32> BoundingBox::getNVertex(const vec3<F32>& normal) const {
    return vec3<F32>(normal.x >= 0.0f ? _min.x : _max.x,
                     normal.y >= 0.0f ? _min.y : _max.y,
                     normal.z >= 0.0f ? _min.z : _max.z);
}

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_
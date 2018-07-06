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

#ifndef _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_
#define _CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_

namespace Divide {

inline bool BoundingBox::ContainsPoint(const vec3<F32>& point) const {
    // const ReadLock r_lock(_lock);
    return (point.x >= _min.x && point.y >= _min.y && point.z >= _min.z &&
            point.x <= _max.x && point.y <= _max.y && point.z <= _max.z);
}

inline bool BoundingBox::Compare(const BoundingBox& bb) const {
    /*ReadLock r_lock(_lock);*/
    return _GUID == bb._GUID;
}

inline bool BoundingBox::operator==(const BoundingBox& B) const {
    return Compare(B);
}

inline bool BoundingBox::operator!=(const BoundingBox& B) const {
    return !Compare(B);
}

inline void BoundingBox::CreateFromPoints(const vectorImpl<vec3<F32>>& points) {
    for (vec3<F32> p : points) {
        Add(p);
    }
}

inline void BoundingBox::Add(const vec3<F32>& v) {
    // WriteLock w_lock(_lock);
    if (v.x > _max.x) _max.x = v.x;
    if (v.x < _min.x) _min.x = v.x;
    if (v.y > _max.y) _max.y = v.y;
    if (v.y < _min.y) _min.y = v.y;
    if (v.z > _max.z) _max.z = v.z;
    if (v.z < _min.z) _min.z = v.z;
    _pointsDirty = true;
};

inline void BoundingBox::Add(const BoundingBox& bb) {
    // WriteLock w_lock(_lock);
    if (bb._max.x > _max.x) _max.x = bb._max.x;
    if (bb._min.x < _min.x) _min.x = bb._min.x;
    if (bb._max.y > _max.y) _max.y = bb._max.y;
    if (bb._min.y < _min.y) _min.y = bb._min.y;
    if (bb._max.z > _max.z) _max.z = bb._max.z;
    if (bb._min.z < _min.z) _min.z = bb._min.z;
    _pointsDirty = true;
}

inline void BoundingBox::Translate(const vec3<F32>& v) {
    // WriteLock w_lock(_lock);
    _min += v;
    _max += v;
    _pointsDirty = true;
}

inline void BoundingBox::Multiply(F32 factor) {
    // WriteLock w_lock(_lock);
    _min *= factor;
    _max *= factor;
    _pointsDirty = true;
}

inline void BoundingBox::Multiply(const vec3<F32>& v) {
    // WriteLock w_lock(_lock);
    _min.x *= v.x;
    _min.y *= v.y;
    _min.z *= v.z;
    _max.x *= v.x;
    _max.y *= v.y;
    _max.z *= v.z;
    _pointsDirty = true;
}

inline void BoundingBox::MultiplyMax(const vec3<F32>& v) {
    // WriteLock w_lock(_lock);
    _max.x *= v.x;
    _max.y *= v.y;
    _max.z *= v.z;
    _pointsDirty = true;
}

inline void BoundingBox::MultiplyMin(const vec3<F32>& v) {
    // WriteLock w_lock(_lock);
    _min.x *= v.x;
    _min.y *= v.y;
    _min.z *= v.z;
    _pointsDirty = true;
}

inline void BoundingBox::setComputed(bool state) { _computed = state; }

inline bool BoundingBox::isComputed() const {
    /*ReadLock r_lock(_lock);*/
    return _computed;
}

inline const vec3<F32>& BoundingBox::getMin() const {
    /*ReadLock r_lock(_lock);*/
    return _min;
}

inline const vec3<F32>& BoundingBox::getMax() const {
    /*ReadLock r_lock(_lock);*/
    return _max;
}

inline vec3<F32> BoundingBox::getCenter() const {
    /*ReadLock r_lock(_lock);*/
    return (_max + _min) * 0.5f;
}

inline vec3<F32> BoundingBox::getExtent() const {
    /*ReadLock r_lock(_lock);*/
    return _max - _min;
}

inline vec3<F32> BoundingBox::getHalfExtent() const {
    /*ReadLock r_lock(_lock);*/
    return (_max - _min) * 0.5f;
}

inline F32 BoundingBox::getWidth() const {
    /*ReadLock r_lock(_lock);*/
    return _max.x - _min.x;
}

inline F32 BoundingBox::getHeight() const {
    /*ReadLock r_lock(_lock);*/
    return _max.y - _min.y;
}

inline F32 BoundingBox::getDepth() const {
    /*ReadLock r_lock(_lock);*/
    return _max.z - _min.z;
}

inline void BoundingBox::setMin(const vec3<F32>& min) {
    /*WriteLock w_lock(_lock);*/
    _min = min;
    _pointsDirty = true;
}

inline void BoundingBox::setMax(const vec3<F32>& max) {
    /*WriteLock w_lock(_lock);*/
    _max = max;
    _pointsDirty = true;
}

inline void BoundingBox::set(const BoundingBox& bb) { set(bb._min, bb._max); }

inline void BoundingBox::set(const vec3<F32>& min, const vec3<F32>& max) {
    /*WriteLock w_lock(_lock);*/
    _min = min;
    _max = max;
    _pointsDirty = true;
}

inline void BoundingBox::reset() {
    /*WriteLock w_lock(_lock);*/
    _min.set(100000.0f, 100000.0f, 100000.0f);
    _max.set(-100000.0f, -100000.0f, -100000.0f);
    _pointsDirty = true;
}

inline const vec3<F32>* BoundingBox::getPoints() const {
    ComputePoints();
    return _points;
}

inline F32 BoundingBox::nearestDistanceFromPoint(const vec3<F32>& pos) const {
    return std::sqrt(nearestDistanceFromPointSquared(pos));
}

inline void BoundingBox::ComputePoints() const {
    if (!_pointsDirty) {
        return;
    }

    /*WriteLock w_lock(_lock);*/
    _points[0].set(_min.x, _min.y, _min.z);
    _points[1].set(_min.x, _min.y, _max.z);
    _points[2].set(_min.x, _max.y, _min.z);
    _points[3].set(_min.x, _max.y, _max.z);
    _points[4].set(_max.x, _min.y, _min.z);
    _points[5].set(_max.x, _min.y, _max.z);
    _points[6].set(_max.x, _max.y, _min.z);
    _points[7].set(_max.x, _max.y, _max.z);
    _pointsDirty = false;
}

};  // namespace Divide

#endif  //_CORE_MATH_BOUNDINGVOLUMES_BOUNDINGBOX_INL_
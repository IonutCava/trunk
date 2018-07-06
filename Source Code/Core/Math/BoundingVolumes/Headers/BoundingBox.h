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

#ifndef BOUNDINGBOX_H_
#define BOUNDINGBOX_H_

#include "Core/Math/Headers/Ray.h"
#include "Utility/Headers/Vector.h"
#include "Utility/Headers/GUIDWrapper.h"
#include "Core/Math/Headers/MathClasses.h"

class BoundingBox : public GUIDWrapper {
public:
    BoundingBox() : GUIDWrapper(),
                    _computed(false),
                    _pointsDirty(true)
    {
        _min.set(std::numeric_limits<F32>::max());
        _max.set(std::numeric_limits<F32>::min());
        _points = new vec3<F32>[8];
    }

    BoundingBox(const vec3<F32>& min, const vec3<F32>& max) : GUIDWrapper(),
                                                              _computed(false),
                                                              _pointsDirty(true),
                                                              _min(min),
                                                              _max(max)
    {
        _points = new vec3<F32>[8];
    }

    BoundingBox(vectorImpl<vec3<F32> >& points) : BoundingBox()
    {
        createFromPoints(points);
    }

    ~BoundingBox()
    {
        SAFE_DELETE_ARRAY(_points);
    }

    BoundingBox(const BoundingBox& b) : GUIDWrapper() {
        //WriteLock w_lock(_lock);
        this->_computed = b._computed;
        this->_min = b._min;
        this->_max = b._max;
        this->_oldMatrix = b._oldMatrix;
        this->_points = new vec3<F32>[8];
        this->_pointsDirty = true;
        memcpy(_points, b._points, sizeof(vec3<F32>) * 8);
    }

    void operator=(const BoundingBox& b){
        //WriteLock w_lock(_lock);
        this->_computed = b._computed;
        this->_min = b._min;
        this->_max = b._max;
        this->_oldMatrix = b._oldMatrix;
        this->_pointsDirty = true;
        memcpy(_points, b._points, sizeof(vec3<F32>) * 8);
    }

    inline bool ContainsPoint(const vec3<F32>& point) const	{
        //const ReadLock r_lock(_lock);
        return (point.x>=_min.x && point.y>=_min.y &&
                point.z>=_min.z && point.x<=_max.x &&
                point.y<=_max.y && point.z<=_max.z);
    }

    inline bool  Collision(const BoundingBox& AABB2) const {
        //ReadLock r_lock(_lock);

        if(this->_max.x < AABB2._min.x) return false;
        if(this->_max.y < AABB2._min.y) return false;
        if(this->_max.z < AABB2._min.z) return false;

        if(this->_min.x > AABB2._max.x) return false;
        if(this->_min.y > AABB2._max.y) return false;
        if(this->_min.z > AABB2._max.z) return false;

        return true;
    }

    inline bool Compare(const BoundingBox& bb) const {
        /*ReadLock r_lock(_lock);*/
        return _GUID == bb._GUID;
    }

    bool operator == (const BoundingBox& B) const { return Compare(B); }
    bool operator != (const BoundingBox& B) const { return !Compare(B); }

    /// Optimized method
    inline bool Intersect(const Ray &r, F32 t0, F32 t1) const {
        //ReadLock r_lock(_lock);
        vec3<F32> bounds[] = {_min, _max};

        F32 t_min  = (bounds[r.sign[0]].x - r.origin.x)   * r.inv_direction.x;
        F32 t_max  = (bounds[1-r.sign[0]].x - r.origin.x) * r.inv_direction.x;
        F32 ty_min = (bounds[r.sign[1]].y - r.origin.y)   * r.inv_direction.y;
        F32 ty_max = (bounds[1-r.sign[1]].y - r.origin.y) * r.inv_direction.y;

        if ( (t_min > ty_max) || (ty_min > t_max) )
            return false;

        if (ty_min > t_min) t_min = ty_min;
        if (ty_max < t_max) t_max = ty_max;

        F32 tz_min = (bounds[r.sign[2]].z - r.origin.z) * r.inv_direction.z;
        F32 tz_max = (bounds[1-r.sign[2]].z - r.origin.z) * r.inv_direction.z;

        if ( (t_min > tz_max) || (tz_min > t_max) )
            return false;

        if (tz_min > t_min) t_min = tz_min;
        if (tz_max < t_max) t_max = tz_max;

        return ( (t_min < t1) && (t_max > t0) );
    }

    inline void createFromPoints(vectorImpl<vec3<F32>>& points)  {
        for (vec3<F32> p : points) {
            Add(p);
        }
    }

    inline void Add(const vec3<F32>& v)	{
        //WriteLock w_lock(_lock);
        if(v.x > _max.x)	_max.x = v.x;
        if(v.x < _min.x)	_min.x = v.x;
        if(v.y > _max.y)	_max.y = v.y;
        if(v.y < _min.y)	_min.y = v.y;
        if(v.z > _max.z)	_max.z = v.z;
        if(v.z < _min.z)	_min.z = v.z;
        _pointsDirty = true;
    };

    inline void Add(const BoundingBox& bb)	{
        //WriteLock w_lock(_lock);
        if(bb._max.x > _max.x)	_max.x = bb._max.x;
        if(bb._min.x < _min.x)	_min.x = bb._min.x;
        if(bb._max.y > _max.y)	_max.y = bb._max.y;
        if(bb._min.y < _min.y)	_min.y = bb._min.y;
        if(bb._max.z > _max.z)	_max.z = bb._max.z;
        if(bb._min.z < _min.z)	_min.z = bb._min.z;
        _pointsDirty = true;
    }

    inline void Translate(const vec3<F32>& v) {
        //WriteLock w_lock(_lock);
        _min += v;
        _max += v;
        _pointsDirty = true;
    }

    inline void Multiply(F32 factor){
        //WriteLock w_lock(_lock);
        _min *= factor;
        _max *= factor;
        _pointsDirty = true;
    }

    inline void Multiply(const vec3<F32>& v){
        //WriteLock w_lock(_lock);
        _min.x *= v.x;
        _min.y *= v.y;
        _min.z *= v.z;
        _max.x *= v.x;
        _max.y *= v.y;
        _max.z *= v.z;
        _pointsDirty = true;
    }

    inline void MultiplyMax(const vec3<F32>& v){
        //WriteLock w_lock(_lock);
        _max.x *= v.x;
        _max.y *= v.y;
        _max.z *= v.z;
        _pointsDirty = true;
    }

    inline void MultiplyMin(const vec3<F32>& v){
        //WriteLock w_lock(_lock);
        _min.x *= v.x;
        _min.y *= v.y;
        _min.z *= v.z;
        _pointsDirty = true;
    }
   
    bool Transform(const BoundingBox& initialBoundingBox, const mat4<F32>& mat, bool force = false){
        //UpgradableReadLock ur_lock(_lock);
        if (!force && _oldMatrix == mat)
            return false;

        _oldMatrix.set(mat);

        const F32* oldMin = &initialBoundingBox._min[0];
        const F32* oldMax = &initialBoundingBox._max[0];

        //UpgradeToWriteLock uw_lock(ur_lock);
        _min.set(mat[12],mat[13],mat[14]);
        _max.set(mat[12],mat[13],mat[14]);

        F32 a, b;
        for (U8 i = 0; i < 3; ++i)	{
            F32& min = _min[i];
            F32& max = _max[i];
            for (U8 j = 0; j < 3; ++j)	{
                a = mat.m[j][i] * oldMin[j];
                b = mat.m[j][i] * oldMax[j]; // Transforms are usually row major

                if (a < b) {
                    min += a;
                    max += b;
                } else {
                    min += b;
                    max += a;
                }
            }
        }
        _pointsDirty = true;
        return true;
    }

    inline void  setComputed(bool state) {
        _computed = state;
    }

    inline bool  isComputed()	 const {
        /*ReadLock r_lock(_lock);*/
        return _computed;
    }

    inline const vec3<F32>&  getMin() const {
        /*ReadLock r_lock(_lock);*/
        return _min;
    }

    inline const vec3<F32>&  getMax() const {
        /*ReadLock r_lock(_lock);*/
        return _max;
    }

    inline vec3<F32>  getCenter()     const {
        /*ReadLock r_lock(_lock);*/
        return (_max + _min)*0.5f;
    }

    inline vec3<F32>  getExtent()     const {
        /*ReadLock r_lock(_lock);*/
        return _max -_min;
    }

    inline vec3<F32>  getHalfExtent() const {
        /*ReadLock r_lock(_lock);*/
        return (_max -_min) * 0.5f;
    }

    inline F32   getWidth()  const {
        /*ReadLock r_lock(_lock);*/
        return _max.x - _min.x;
    }

    inline F32   getHeight() const {
        /*ReadLock r_lock(_lock);*/
        return _max.y - _min.y;
    }

    inline F32   getDepth()  const {
        /*ReadLock r_lock(_lock);*/
        return _max.z - _min.z;
    }

    inline void setMin(const vec3<F32>& min)   {
        /*WriteLock w_lock(_lock);*/
        _min = min;
        _pointsDirty = true;
    }

    inline void setMax(const vec3<F32>& max)   {
        /*WriteLock w_lock(_lock);*/
        _max = max;
        _pointsDirty = true;
    }

    inline void set(const BoundingBox& bb){
        set(bb._min, bb._max);
    }

    inline void set(const vec3<F32>& min, const vec3<F32>& max)  {
        /*WriteLock w_lock(_lock);*/
        _min = min;
        _max = max;
        _pointsDirty = true;
    }

    inline void reset() {
        /*WriteLock w_lock(_lock);*/
        _min.set(100000.0f, 100000.0f, 100000.0f);
        _max.set(-100000.0f, -100000.0f, -100000.0f);
        _pointsDirty = true;
    }

    inline const vec3<F32>* getPoints() const {
        ComputePoints();
        return _points;
    }

    inline F32 nearestDistanceFromPointSquared( const vec3<F32> &pos ) const {
        const vec3<F32>& center = getCenter();
        const vec3<F32>& hextent = getHalfExtent();
        _cacheVector.set(std::max( 0.0f, fabsf( pos.x - center.x ) - hextent.x ),
                         std::max(0.0f, fabsf(pos.y - center.y) - hextent.y),
                         std::max(0.0f, fabsf(pos.z - center.z) - hextent.z));
        return _cacheVector.lengthSquared();
    }

    inline F32 nearestDistanceFromPoint( const vec3<F32> &pos) const {
        return std::sqrtf(nearestDistanceFromPointSquared(pos));
    }

protected:

    void ComputePoints() const {
        if(!_pointsDirty)
            return;
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

private:
    bool _computed;
    vec3<F32> _min, _max;
    mat4<F32> _oldMatrix;

    // This is is very limited in scope so mutable should be ok
    mutable bool _pointsDirty;
    mutable vec3<F32> *_points;
    mutable vec3<F32> _cacheVector;
    //mutable SharedLock _lock;
};

#endif

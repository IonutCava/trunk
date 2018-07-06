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

#ifndef BOUNDINGSPHERE_H_
#define BOUNDINGSPHERE_H_
#include "Core/Math/Headers/Ray.h"

namespace Divide {

class BoundingSphere {
public:
	BoundingSphere() : _computed(false),
				       _visibility(false),
				       _dirty(true),
					   _radius(0.0f)
	{
		_center.reset();
	}

	BoundingSphere(const vec3<F32>& center, F32 radius) : _computed(false),
														  _visibility(false),
														  _dirty(true),
														  _center(center),
														  _radius(radius)
	{
	}

    BoundingSphere(vectorImpl<vec3<F32> >& points) : BoundingSphere()
    {
        createFromPoints(points);
    }

	BoundingSphere(const BoundingSphere& s){
		//WriteLock w_lock(_lock);
		this->_computed = s._computed;
		this->_visibility = s._visibility;
		this->_dirty = s._dirty;
		this->_center = s._center;
		this->_radius = s._radius;
	}

	void operator=(const BoundingSphere& s){
		//WriteLock w_lock(_lock);
		this->_computed = s._computed;
		this->_visibility = s._visibility;
		this->_dirty = s._dirty;
		this->_center = s._center;
		this->_radius = s._radius;
	}

	inline void fromBoundingBox(const BoundingBox& bBox){
		_center = bBox.getCenter();
		_radius = (bBox.getMax()-_center).length();
	}

    //https://code.google.com/p/qe3e/source/browse/trunk/src/BoundingSphere.h?r=28
    void add(const BoundingSphere& bSphere){
        F32 dist = (bSphere._center - _center).length();

        if (_radius >= dist + bSphere._radius)
            return;

        if (bSphere._radius >= dist + _radius) {
            _center = bSphere._center;
            _radius = bSphere._radius;
        }

        F32 nRadius = (_radius + dist + bSphere._radius)*0.5;
        F32 ratio = (nRadius - _radius) / dist;

        _center += (bSphere._center - _center) * ratio;

        _radius = nRadius;
    }

    void addRadius(const BoundingSphere& bSphere){
        F32 dist = (bSphere._center - _center).length() + bSphere._radius;
        if (_radius < dist)
            _radius = dist;
    }

    void add(const vec3<F32>& point){
        vec3<F32> diff = point - _center;
        F32 dist = diff.length();
        if (_radius < dist) {
            F32 nRadius = (dist - _radius) * 0.5f;
            _center += diff*(nRadius / dist);
            _radius += nRadius;
       }
    }

    void addRadius(const vec3<F32>& point){
        F32 dist = (point - _center).length();
        if (_radius < dist)
            _radius = dist;
    }

    void createFromPoints(vectorImpl<vec3<F32>>& points)  {
        _radius = 0;
        I32 numPoints = (I32)points.size();

        for(vec3<F32> p : points) {
            _center += p / numPoints;
        }

        for (vec3<F32> p : points) {
            F32 distance = (p - _center).length();

            if (distance > _radius)
                _radius = distance;
        }
    }


    inline void setRadius(F32 radius) {_radius = radius;}
    inline void setCenter(const vec3<F32>& center) {_center = center;}

	inline const vec3<F32>& getCenter()   const {return _center;}
	inline F32              getRadius()   const {return _radius;}
    inline F32              getDiameter() const {return _radius * 2;}

private:
	bool _computed, _visibility,_dirty;
	vec3<F32> _center;
	F32 _radius;
	mutable SharedLock _lock;
};

}; //namespace Divide

#endif
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

	void fromBoundingBox(const BoundingBox& bBox){
		_center = bBox.getCenter();
		_radius = (bBox.getMax()-_center).length();
	}

	const vec3<F32>& getCenter() const {return _center;}
	F32              getRadius() const {return _radius;}

private:
	bool _computed, _visibility,_dirty;
	vec3<F32> _center;
	F32 _radius;
	mutable SharedLock _lock;
};

#endif
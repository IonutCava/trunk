/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
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

	void fromBoundingBox(BoundingBox& bBox){
		_center = bBox.getCenter();
		_radius = (bBox.getMax()-_center).length();
	};

	const vec3<F32>& getCenter() const {return _center;}
	F32              getRadius() const {return _radius;}

private:
	bool _computed, _visibility,_dirty;
	vec3<F32> _center;
	F32 _radius;
	mutable SharedLock _lock;
};

#endif
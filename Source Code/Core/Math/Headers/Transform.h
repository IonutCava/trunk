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

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include <boost/atomic.hpp>
#include "Quaternion.h"

class Transform {

public:
	Transform()	: _dirty(true){
		WriteLock w_lock(_lock);
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
		_parentMatrix.identity();
		_globalMatrix.identity();
		_scale = vec3<F32>(1,1,1);
	}

	Transform(const Quaternion<F32>& orientation, const vec3<F32>& translation, const vec3<F32>& scale) : 
			  _orientation(orientation), _translation(translation), _scale(scale), _dirty(true){
		WriteLock w_lock(_lock);
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
		_parentMatrix.identity();
		_globalMatrix.identity();
	}

	void setPosition(const vec3<F32>& position){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
	}

	void setPositionX(F32 position){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.x = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
	}

	void setPositionY(F32 position){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.y = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
	}
	
	void setPositionZ(F32 position){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.z = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
	}

	void translate(const vec3<F32>& position){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation  += position;
		_translationMatrix.translate(_translation);
	}

	void translateX(const F32 positionX){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.x += positionX;
		_translationMatrix.translate(_translation); 
	}

	void translateY(const F32 positionY){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.y += positionY;
		_translationMatrix.translate(_translation); 
	}

	void translateZ(const F32 positionZ){
        _dirty = true;
		WriteLock w_lock(_lock);
		_translation.z += positionZ;
		_translationMatrix.translate(_translation);
	}

	void scale(const vec3<F32>& scale){
        _dirty = true;
		WriteLock w_lock(_lock);
		_scaleMatrix.scale(scale);
		_scale = scale; 
	}

	inline void scale(const F32 scale)  {this->scale(vec3<F32>(scale,scale,scale)); }
	inline void scaleX(const F32 scale) {this->scale(vec3<F32>(scale,_scale.y,_scale.z));}
	inline void scaleY(const F32 scale) {this->scale(vec3<F32>(_scale.x,scale,_scale.z));}
	inline void scaleZ(const F32 scale) {this->scale(vec3<F32>(_scale.x,_scale.y,scale));}

	void rotate(const vec3<F32>& axis, F32 degrees){
        _dirty = true;
		WriteLock w_lock(_lock);
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis;
		_angle = degrees;
	}

	void rotateEuler(const vec3<F32>& euler){
        _dirty = true;
		WriteLock w_lock(_lock);
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix();
	}

	void rotateQuaternion(const Quaternion<F32>& quat){
        _dirty = true;
		WriteLock w_lock(_lock);
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
	}

	inline void rotateX(F32 angle){this->rotate(vec3<F32>(1,0,0),angle);}
	inline void rotateY(F32 angle){this->rotate(vec3<F32>(0,1,0),angle);}
	inline void rotateZ(F32 angle){this->rotate(vec3<F32>(0,0,1),angle);}

	inline mat4<F32> const& getRotationMatrix()       {ReadLock r_lock(_lock); return _orientation.getMatrix();}
	inline mat4<F32> const& getParentMatrix()   const {ReadLock r_lock(_lock); return _parentMatrix;}
	inline vec3<F32> const& getPosition()       const {ReadLock r_lock(_lock); return _translation;}
	inline vec3<F32> const& getScale()          const {ReadLock r_lock(_lock); return _scale;}

	inline Quaternion<F32> const& getOrientation() const {ReadLock r_lock(_lock); return _orientation;}

	inline mat4<F32> const& getMatrix()       {this->applyTransforms(); 
											   ReadLock r_lock(_lock);
											   return this->_worldMatrix;}
	inline mat4<F32> const& getGlobalMatrix() {this->applyTransforms();
	                                           ReadLock r_lock(_lock);
											   return this->_globalMatrix;}

	void applyTransforms(){
		if(!_dirty) return;

		WriteLock w_lock(_lock);
		_worldMatrix.identity();
		_worldMatrix *= _translationMatrix;
		_worldMatrix *= _rotationMatrix;
		_worldMatrix *= _scaleMatrix;
		_globalMatrix = _worldMatrix * _parentMatrix; 
		this->clean();
	}

	void setTransforms(const mat4<F32>& transform){
		WriteLock w_lock(_lock);
		_worldMatrix = transform;
		this->clean();
	}

	void setParentMatrix(const mat4<F32>& transform){
		_dirty = true;
        WriteLock w_lock(_lock);
		_parentMatrix = transform;
	}

	bool compare(Transform* t){
		ReadLock r_lock(_lock);
		if(_scale.compare(t->_scale) &&  
		   _orientation.compare(t->_orientation) &&
		   _translation.compare(t->_translation))
		   return true;

		return false;
	}

	inline bool isDirty() {return _dirty;}

private:
	inline void clean()   {_dirty = false;} 

private:
	Quaternion<F32> _orientation;
	vec3<F32> _axis;
	F32 _angle;
	vec3<F32> _translation;
	vec3<F32> _scale;
	mat4<F32> _worldMatrix;
	mat4<F32> _parentMatrix;
	mat4<F32> _globalMatrix; /// world * parent
	mat4<F32> _scaleMatrix,_rotationMatrix,_translationMatrix;
    mutable boost::atomic<bool> _dirty;
	mutable SharedLock _lock;
};

#endif
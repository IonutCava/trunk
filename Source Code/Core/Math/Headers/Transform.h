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

#include "Quaternion.h"

class Transform {

public:
	Transform()	: _dirty(true){
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
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
		_parentMatrix.identity();
		_globalMatrix.identity();
	}

	void setPosition(const vec3<F32>& position){
		WriteLock w_lock(_lock);
		_translation = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionX(F32 position){
		WriteLock w_lock(_lock);
		_translation.x = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionY(F32 position){
		WriteLock w_lock(_lock);
		_translation.y = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}
	
	void setPositionZ(F32 position){
		WriteLock w_lock(_lock);
		_translation.z = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translate(const vec3<F32>& position){
		WriteLock w_lock(_lock);
		_translation  += position;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translateX(const F32 positionX){
		WriteLock w_lock(_lock);
		_translation.x += positionX;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateY(const F32 positionY){
		WriteLock w_lock(_lock);
		_translation.y += positionY;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateZ(const F32 positionZ){
		WriteLock w_lock(_lock);
		_translation.z += positionZ;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void scale(const vec3<F32>& scale){
		WriteLock w_lock(_lock);
		_scaleMatrix.scale(scale);
		_scale = scale; 
		_dirty = true;
	}

	inline void scale(const F32 scale)  {this->scale(vec3<F32>(scale,scale,scale)); }
	inline void scaleX(const F32 scale) {this->scale(vec3<F32>(scale,_scale.y,_scale.z));}
	inline void scaleY(const F32 scale) {this->scale(vec3<F32>(_scale.x,scale,_scale.z));}
	inline void scaleZ(const F32 scale) {this->scale(vec3<F32>(_scale.x,_scale.y,scale));}

	void rotate(const vec3<F32>& axis, F32 degrees){
		WriteLock w_lock(_lock);
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis;
		_angle = degrees;
		_dirty = true;
	}

	void rotateEuler(const vec3<F32>& euler){
		WriteLock w_lock(_lock);
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateQuaternion(const Quaternion<F32>& quat){
		WriteLock w_lock(_lock);
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
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
		UpgradableReadLock ur_lock(_lock);
		if(!_dirty) return;
		UpgradeToWriteLock uw_lock(ur_lock);

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
		WriteLock w_lock(_lock);
		_dirty = true;
		_parentMatrix = transform;
	}

	bool compare(Transform* t){
		bool result = false;
		ReadLock r_lock(_lock);
		if(_scale.compare(t->_scale) &&  
		   _orientation.compare(t->_orientation) &&
		   _translation.compare(t->_translation))
		   result = true;

		return result;
	}

	inline bool isDirty() {ReadLock r_lock(_lock); return _dirty;}

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
	bool _dirty;
	mutable Lock _lock;
};

#endif
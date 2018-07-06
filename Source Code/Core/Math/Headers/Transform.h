/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

class Transform
{
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
		_translation = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionX(F32 position){
		_translation.x = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionY(F32 position){
		_translation.y = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}
	
	void setPositionZ(F32 position){
		_translation.z = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translate(const vec3<F32>& position){
		_translation  += position;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translateX(const F32 positionX){
		_translation.x += positionX;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateY(const F32 positionY){
		_translation.y += positionY;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateZ(const F32 positionZ){
		_translation.z += positionZ;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void scale(const vec3<F32>& scale){
		_scaleMatrix.scale(scale);
		_scale = scale; 
		_dirty = true;
	}

	inline void scale(const F32 scale)  {this->scale(vec3<F32>(scale,scale,scale)); }
	inline void scaleX(const F32 scale) {this->scale(vec3<F32>(scale,_scale.y,_scale.z));}
	inline void scaleY(const F32 scale) {this->scale(vec3<F32>(_scale.x,scale,_scale.z));}
	inline void scaleZ(const F32 scale) {this->scale(vec3<F32>(_scale.x,_scale.y,scale));}

	void rotate(const vec3<F32>& axis, F32 degrees){
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis;
		_angle = degrees;
		_dirty = true;
	}

	void rotateEuler(const vec3<F32>& euler){
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateQuaternion(const Quaternion<F32>& quat){
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	inline void rotateX(F32 angle){this->rotate(vec3<F32>(1,0,0),angle);}
	inline void rotateY(F32 angle){this->rotate(vec3<F32>(0,1,0),angle);}
	inline void rotateZ(F32 angle){this->rotate(vec3<F32>(0,0,1),angle);}
	inline mat4<F32> const& getRotationMatrix() { return _orientation.getMatrix();}
	inline mat4<F32> const& getParentMatrix()   const {return _parentMatrix;}
	inline vec3<F32> const& getPosition()       const {return _translation;}
	inline vec3<F32> const& getScale()          const {return _scale;}

	inline Quaternion<F32> const& getOrientation() const {return _orientation;}
	inline mat4<F32> const& getMatrix()       {this->applyTransforms(); return this->_worldMatrix;}
	inline mat4<F32> const& getGlobalMatrix() {this->applyTransforms(); return this->_globalMatrix;}

	void applyTransforms(){
		if(isDirty()){
			_worldMatrix.identity();
			_worldMatrix *= _translationMatrix;
			_worldMatrix *= _rotationMatrix;
			_worldMatrix *= _scaleMatrix;
			_globalMatrix = _worldMatrix * _parentMatrix; 
			this->clean();
		}
	}

	void setTransforms(const mat4<F32>& transform){
		_worldMatrix = transform;
		this->clean();
	}

	void setParentMatrix(const mat4<F32>& transform){
		_dirty = true;
		_parentMatrix = transform;
	}

	bool compare(Transform* t){
		bool result = false;

		if(_scale.compare(t->_scale) &&  
		   _orientation.compare(t->_orientation) &&
		   _translation.compare(t->_translation))
		   result = true;

		return result;
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
	bool _dirty;
};

#endif
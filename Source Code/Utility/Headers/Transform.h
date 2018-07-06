/*“Copyright 2009-2011 DIVIDE-Studio”*/
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
	Transform()	: _dirty(false){
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
		_parentMatrix.identity();
	}

	Transform(const Quaternion& orientation, const vec3& translation, const vec3& scale) : 
			  _orientation(orientation), _translation(translation), _scale(scale), _dirty(false){
		_dirty = true;
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
		_parentMatrix.identity();
	}

	void setPosition(const vec3& position){
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

	void translate(const vec3& position){
		_translation   += position;
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

	void scale(const vec3& scale){
		_scaleMatrix.scale(scale);
		_scale = scale; 
		_dirty = true;
	}

	void scale(const F32 scale)  {this->scale(vec3(scale,scale,scale)); }
	void scaleX(const F32 scale) {this->scale(vec3(scale,_scale.y,_scale.z));}
	void scaleY(const F32 scale) {this->scale(vec3(_scale.x,scale,_scale.z));}
	void scaleZ(const F32 scale) {this->scale(vec3(_scale.x,_scale.y,scale));}

	void rotate(const vec3& axis, F32 degrees){
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis;
		_angle = degrees;
		_dirty = true;
	}

	void rotateEuler(const vec3& euler){
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateQuaternion(const Quaternion& quat){
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateX(F32 angle){this->rotate(vec3(1,0,0),angle);}
	void rotateY(F32 angle){this->rotate(vec3(0,1,0),angle);}
	void rotateZ(F32 angle){this->rotate(vec3(0,0,1),angle);}
	

	const mat4& getMatrix()            {this->applyTransforms(); return _worldMatrix;}
	const mat4& getParentMatrix()      {return _parentMatrix;}
	const mat4& getRotationMatrix()    {return _orientation.getMatrix();}
	const vec3& getPosition()          {return _translation;}
	const vec3& getScale()             {return _scale;}
	const Quaternion& getOrientation() {return _orientation;}

	void applyTransforms(){
		if(isDirty()){
			_worldMatrix.identity();
			_worldMatrix *= _translationMatrix;
			_worldMatrix *= _rotationMatrix;
			_worldMatrix *= _scaleMatrix;
			this->clean();
		}
	}

	void setTransforms(const mat4& transform){
		_worldMatrix = transform;
		this->clean();
	}

	void setParentMatrix(const mat4& transform){
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

	bool isDirty() {return _dirty;}
private:
	void clean()   {_dirty = false;} 

private:
	Quaternion _orientation;
	vec3 _axis;
	F32 _angle;
	vec3 _translation;
	vec3 _scale;
	mat4 _worldMatrix,_scaleMatrix,_rotationMatrix,_translationMatrix,_parentMatrix;
	bool _dirty;
};

#endif
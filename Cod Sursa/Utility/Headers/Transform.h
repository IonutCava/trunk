#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "Quaternion.h"

class Transform
{
public:
	Transform()
	{
		_dirty = false;
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
	}

	Transform(const Quaternion& orientation, const vec3& translation, const vec3& scale) : 
			  _orientation(orientation), _translation(translation), _scale(scale)
	{
		_dirty = true;
		_worldMatrix.identity();
		_rotationMatrix.identity();
		_scaleMatrix.identity();
		_translationMatrix.identity();
	}

	void setPosition(const vec3& position) 
	{
		_translation = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionX(F32 position)
	{
		_translation.x = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void setPositionY(F32 position)
	{
		_translation.y = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}
	
	void setPositionZ(F32 position)
	{
		_translation.z = position;
		_translationMatrix.identity();
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translate(const vec3& position)    
	{
		_translation   += position;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void translateX(const F32 positionX)
	{
		_translation.x += positionX;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateY(const F32 positionY)
	{
		_translation.y += positionY;
		_translationMatrix.translate(_translation); 
		_dirty = true;
	}

	void translateZ(const F32 positionZ)
	{
		_translation.z += positionZ;
		_translationMatrix.translate(_translation);
		_dirty = true;
	}

	void scale(const vec3& scale)
	{
		_scaleMatrix.scale(scale);
		_scale = scale; 
		_dirty = true;
	}

	void scale(const F32 scale){ this->scale(vec3(scale,scale,scale)); }

	void rotate(const vec3& axis, F32 degrees)
	{
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis;
		_angle = degrees;
		_dirty = true;
	}

	void rotateEuler(const vec3& euler)
	{
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateQuaternion(const Quaternion& quat)
	{
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
		_dirty = true;
	}

	void rotateX(F32 angle){_rotationMatrix.rotate_x(angle); _orientation.FromAxis(vec3(1,0,0),angle); _dirty = true;}
	void rotateY(F32 angle){_rotationMatrix.rotate_y(angle); _orientation.FromAxis(vec3(1,0,0),angle); _dirty = true;}
	void rotateZ(F32 angle){_rotationMatrix.rotate_z(angle); _orientation.FromAxis(vec3(1,0,0),angle); _dirty = true;}
	

	const mat4& getMatrix() {if(isDirty()) applyTransforms(); return _worldMatrix;}
	const mat4& getRotationMatrix() {return _orientation.getMatrix();}
	const vec3& getPosition() {return _translation;}
	const vec3& getScale() {return _scale;}
	const Quaternion& getOrientation() {return _orientation;}

	void applyTransforms()
	{
		_worldMatrix.identity();
		_worldMatrix *= _translationMatrix;
		_worldMatrix *= _rotationMatrix;
		_worldMatrix *= _scaleMatrix;
		clean();
	}

	void setTransforms(const mat4& transform)
	{
		_worldMatrix = transform;
		clean();
	}

private:
	bool isDirty() {return _dirty;}
	void clean()   {_dirty = false;} 

private:
	Quaternion _orientation;
	vec3 _axis;
	F32 _angle;
	vec3 _translation;
	vec3 _scale;
	mat4 _worldMatrix,_scaleMatrix,_rotationMatrix,_translationMatrix;
	bool _dirty;
};

#endif
#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "MathClasses.h"
#include "Quaternion.h"

class Transform
{
public:
	Transform(const Quaternion& orientation, const vec3& translation, const vec3& scale) : 
	  _orientation(orientation), _translation(translation), _scale(scale)
	 {
		 _dirty = false;
		 _worldMatrix.identity();
		 _rotationMatrix.identity();
		 _scaleMatrix.identity();
		 _translationMatrix.identity();
	  }
	  ~Transform(){};

	void translate(const vec3& position) {_translationMatrix.translate(position); _translation = position;    _dirty = true;}
	void translateX(float positionX)	 {_translation.x = positionX; _translationMatrix.translate(_translation); _dirty = true;}
	void translateY(float positionY)	 {_translation.y = positionY; _translationMatrix.translate(_translation); _dirty = true;}
	void translateZ(float positionZ)	 {_translation.z = positionZ; _translationMatrix.translate(_translation); _dirty = true;}

	void scale(const vec3& scale) {_scaleMatrix.scale(scale); _scale = scale; _dirty = true;}

	void rotate(const vec3& axis, float degrees)
	{
		_orientation.FromAxis(axis,degrees);
		_rotationMatrix = _orientation.getMatrix();
		_axis = axis; _angle = degrees;_dirty = true;
	}
	void rotateEuler(const vec3& euler)
	{
		_orientation.FromEuler(euler);
		_orientation.getAxisAngle(&_axis,&_angle,true);
		_rotationMatrix = _orientation.getMatrix(); _dirty = true;
	}

	void rotateQuaternion(const Quaternion& quat)
	{
		_orientation = quat; 
		_rotationMatrix = _orientation.getMatrix();
	}

	void rotateX(float angle){_rotationMatrix.rotate_x(angle); _axis.set(1,0,0); angle = angle; _dirty = true;}
	void rotateY(float angle){_rotationMatrix.rotate_y(angle); _axis.set(0,1,0); angle = angle; _dirty = true;}
	void rotateZ(float angle){_rotationMatrix.rotate_z(angle); _axis.set(0,0,1); angle = angle; _dirty = true;}
	

	const mat4& getMatrix() {if(isDirty()) applyTransforms(); return _worldMatrix;}

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

private:
	bool isDirty() {return _dirty;}
	void clean()   {_dirty = false;} 

private:
	Quaternion _orientation;
	vec3 _axis;
	float _angle;
	vec3 _translation;
	vec3 _scale;
	mat4 _worldMatrix,_scaleMatrix,_rotationMatrix,_translationMatrix;
	bool _dirty;
};

#endif
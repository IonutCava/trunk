/*
   Copyright (c) 2013 DIVIDE-Studio
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


#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include <boost/atomic.hpp>
#include "Quaternion.h"

class Transform {
public:

	Transform();
	Transform(const Quaternion<F32>& orientation, const vec3<F32>& translation, const vec3<F32>& scale);
	~Transform();

	void translate(const vec3<F32>& position)          { _dirty = true; WriteLock w_lock(_lock); _translation += position; }
	void setPosition(const vec3<F32>& position)        { _dirty = true; WriteLock w_lock(_lock); _translation = position;  }
	void scale(const vec3<F32>& scale)                 { _dirty = true; WriteLock w_lock(_lock); _scale = scale; _rebuildMatrix = true; }
    void rotate(const vec3<F32>& axis, F32 degrees)    { _dirty = true; WriteLock w_lock(_lock); _orientation.FromAxis(axis,degrees);_rebuildMatrix = true; }
	void rotateEuler(const vec3<F32>& euler)           { _dirty = true; WriteLock w_lock(_lock); _orientation.FromEuler(euler);      _rebuildMatrix = true; }
	void rotateQuaternion(const Quaternion<F32>& quat) { _dirty = true; WriteLock w_lock(_lock); _orientation = quat;                _rebuildMatrix = true;}

	///Helper functions
	inline void scale(const F32 scale)            {this->scale(vec3<F32>(scale,scale,scale)); }
	inline void scaleX(const F32 scale)           {this->scale(vec3<F32>(scale,_scale.y,_scale.z));}
	inline void scaleY(const F32 scale)           {this->scale(vec3<F32>(_scale.x,scale,_scale.z));}
	inline void scaleZ(const F32 scale)           {this->scale(vec3<F32>(_scale.x,_scale.y,scale));}
	inline void rotateX(const F32 angle)          {this->rotate(vec3<F32>(1,0,0),angle);}
	inline void rotateY(const F32 angle)          {this->rotate(vec3<F32>(0,1,0),angle);}
	inline void rotateZ(const F32 angle)          {this->rotate(vec3<F32>(0,0,1),angle);}
	inline void translateX(const F32 positionX)   {this->translate(vec3<F32>(positionX,0.0f,0.0f));}
	inline void translateY(const F32 positionY)   {this->translate(vec3<F32>(0.0f,positionY,0.0f));}
	inline void translateZ(const F32 positionZ)   {this->translate(vec3<F32>(0.0f,0.0f,positionZ));}
	inline void setPositionX(const F32 positionX) {this->setPosition(vec3<F32>(positionX,_translation.y,_translation.z));}
	inline void setPositionY(const F32 positionY) {this->setPosition(vec3<F32>(_translation.x,positionY,_translation.z));}
	inline void setPositionZ(const F32 positionZ) {this->setPosition(vec3<F32>(_translation.x,_translation.y,positionZ));}

	inline const vec3<F32>&       getScale()          const {ReadLock r_lock(_lock); return _scale;}
	inline const vec3<F32>&       getPosition()       const {ReadLock r_lock(_lock); return _translation;}
	inline const mat4<F32>&       getParentMatrix()   const {ReadLock r_lock(_lock); return _parentMatrix;}
	inline const Quaternion<F32>& getOrientation()    const {ReadLock r_lock(_lock); return _orientation;}
	inline const mat4<F32>&       getRotationMatrix()       {ReadLock r_lock(_lock); return _orientation.getMatrix();}

	inline const mat4<F32>& getMatrix()       {this->applyTransforms(); ReadLock r_lock(_lock); return _worldMatrix; }
	inline       mat4<F32>  getGlobalMatrix() {this->applyTransforms(); ReadLock r_lock(_lock);	return this->_worldMatrix * this->_parentMatrix; }

	inline void setTransforms(const mat4<F32>& transform)   {WriteLock w_lock(_lock); _worldMatrix = transform; this->clean(); }
	inline void setParentMatrix(const mat4<F32>& transform) {_dirty = true; WriteLock w_lock(_lock); _parentMatrix = transform; }

	void applyTransforms();
	bool compare(Transform* t);

	inline bool isDirty()         const {return _dirty;}
    inline bool isUniformScaled() const {return _scale.isUniform();}

private:
	inline void clean()   {_dirty = false; _rebuildMatrix = false;}

private:
	///The object's position in the world as a 3 component vector
	vec3<F32> _translation;
	///Scaling is stored as a 3 component vector. This helps us check more easily if it's an uniform scale or not
	vec3<F32> _scale;
	///This is the actual model matrix, but it will not convert to world space as it depends on it's parent in graph
	mat4<F32> _worldMatrix;
	///This is our parent matrix to help us get our world space position
	mat4<F32> _parentMatrix;
	///All orientation/rotation info is stored in a Quaternion (because they are awesome and also have an internal mat4 if needed)
	Quaternion<F32> _orientation;
	///_dirty is set to true whenever a translation, rotation or scale is applied
    boost::atomic_bool _dirty;
	///_rebuildMatrix is set to true only when a rotation or scale is applied to avoid rebuilding matrices on translation only
	boost::atomic_bool _rebuildMatrix;

	mutable SharedLock _lock;
};

#endif
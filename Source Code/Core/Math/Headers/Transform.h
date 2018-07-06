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

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include <boost/atomic.hpp>
#include <boost/noncopyable.hpp>
#include "Quaternion.h"
#include "Utility/Headers/GUIDWrapper.h"
   
class Transform : public GUIDWrapper, private boost::noncopyable {
public:

    Transform();
    Transform(const Quaternion<F32>& orientation, const vec3<F32>& translation, const vec3<F32>& scale);

    ~Transform();

    void translate(const vec3<F32>& position)          { WriteLock w_lock(_lock); setDirty(); _translation += position; }
    void setPosition(const vec3<F32>& position)        { WriteLock w_lock(_lock); setDirty(); _translation = position;  }
    void scale(const vec3<F32>& scale)                 { WriteLock w_lock(_lock); setDirty(); _scale = scale;                           rebuildMatrix(); }
    void rotate(const vec3<F32>& axis, F32 degrees)    { WriteLock w_lock(_lock); setDirty(); _orientation.fromAxisAngle(axis,degrees); rebuildMatrix(); }
    void rotateEuler(const vec3<F32>& euler)           { WriteLock w_lock(_lock); setDirty(); _orientation.fromEuler(euler);            rebuildMatrix(); }

    void rotate(Quaternion<F32> quat) {
        WriteLock w_lock(_lock); 
        setDirty(); 
        quat.normalize();
        _orientation = _orientation * quat;
        rebuildMatrix();
    }

    void rotateSlerp(const Quaternion<F32>& quat, const D32 deltaTime) {
        WriteLock w_lock(_lock); 
        setDirty(); 
        _orientation.slerp(quat, deltaTime); 
        rebuildMatrix();
    }

    ///Helper functions
    inline bool isDirty()         const {
        if(hasParentTransform())
            return _dirty || _parentTransform->isDirty();

        return  _dirty;
    }

    
    inline bool isUniformScaled() const {
        if(hasParentTransform()){
            ReadLock r_lock(_parentLock);
            return getLocalScale().isUniform() && _parentTransform->isUniformScaled();
        }

        return getLocalScale().isUniform();
    }

    inline bool isPhysicsDirty()  const {return _physicsDirty;}
    ///Transformation helper functions. These just call the normal translate/rotate/scale functions
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
    ///Transformation helper functions. These return the curent, local scale, position and orientation
    inline const vec3<F32>&       getLocalScale()       const {ReadLock r_lock(_lock); return _scale; }
    inline const vec3<F32>&       getLocalPosition()    const {ReadLock r_lock(_lock); return _translation; }
    inline const Quaternion<F32>& getLocalOrientation() const {ReadLock r_lock(_lock); return _orientation; }

    inline vec3<F32> getScale() const {
        if(hasParentTransform()){
            ReadLock r_lock(_parentLock);
            vec3<F32> parentScale = _parentTransform->getScale();
            r_lock.unlock();
            return  parentScale * getLocalScale();
        }

        return getLocalScale();
    }

    inline vec3<F32> getPosition() const {
        if(hasParentTransform()){
            ReadLock r_lock(_parentLock);
            vec3<F32> parentPos = _parentTransform->getPosition();
            r_lock.unlock();
            return parentPos + getLocalPosition();
        }

        return getLocalPosition();
    }

    inline Quaternion<F32> getOrientation() const {
         if(hasParentTransform()){
            ReadLock r_lock(_parentLock);
            Quaternion<F32> parentOrientation = _parentTransform->getOrientation();
            r_lock.unlock();
            return parentOrientation.inverse() * getLocalOrientation();
        }

        return getLocalOrientation();
    }

    ///Get the local transformation matrix
    inline const mat4<F32>&  getMatrix() { return this->applyTransforms(); }
    ///Get the absolute transformation matrix. The local matrix with the parent's transforms applied
    inline mat4<F32>  getGlobalMatrix()  { return this->getMatrix() * this->getParentMatrix(); }
    ///Get the parent's transformation matrix if it exists
    inline mat4<F32> getParentMatrix() const {
        if(hasParentTransform()){
            ReadLock r_lock(_parentLock);
            return _parentTransform->getGlobalMatrix();
        }
        return mat4<F32>(); //identity
    }

    void setRotation(const Quaternion<F32>& quat) { 
        WriteLock w_lock(_lock);
        setDirty(); 
        _orientation = quat;  
        rebuildMatrix(); 
    }

    ///Sets the transform to match a certain transformation matrix.
    ///Scale, orientation and translation are extracted from the specified matrix
    inline void setTransforms(const mat4<F32>& transform) {
        WriteLock w_lock(_lock);
        setDirty();
        Util::Mat4::decompose(transform, _scale, _orientation, _translation);
    }

    ///Get the parent's global transformation
    inline Transform* const getParentTransform() const {
        if(hasParentTransform()){ //<avoid an extra mutex lock
            ReadLock r_lock(_parentLock);
            return _parentTransform;
        }
        return NULL;
    }
    ///Set the parent's global transform (the parent's transform with its parent's transform applied and so on)
    inline bool setParentTransform(Transform* transform) {
        WriteLock w_lock(_parentLock);

        if(!_parentTransform && !transform) return false;
        if(_parentTransform && transform && _parentTransform->getGUID() == transform->getGUID()) return false;

        _parentTransform = transform;
        _hasParentTransform = (transform != NULL);
        return true;
    }

    inline void clone(Transform* const transform){
        WriteLock w_lock(_lock);
        _scale.set(transform->getLocalScale());
        _translation.set(transform->getLocalPosition());
        _orientation.set(transform->getLocalOrientation());
        setDirty();
        w_lock.unlock();
        setParentTransform(transform->getParentTransform());
    }
    
    const mat4<F32>& interpolate(Transform* const transform, const D32 factor);

    ///Creates the local transformation matrix using the position, scale and position values
    const mat4<F32>& applyTransforms();
    ///Compares 2 transforms
    bool compare(const Transform* const t);
    ///Call this when the physics actor is updated
    inline void cleanPhysics()  {_physicsDirty = false;}
    ///Reset transform to identity
           void identity();
private:
    
    inline void clean()         {_dirty = false; _rebuildMatrix = false;}
    inline void setDirty()      {_dirty = true; _physicsDirty = true;}
    inline void rebuildMatrix() {_rebuildMatrix = true;}
    inline bool hasParentTransform() const {
#ifdef _DEBUG
        //ReadLock r_lock(_parentLock);
        assert(_hasParentTransform && _parentTransform != NULL || !_hasParentTransform && _parentTransform == NULL); 
#endif
        return _hasParentTransform;
    }

private:
    ///The object's position in the world as a 3 component vector
    vec3<F32> _translation;
    ///Scaling is stored as a 3 component vector. This helps us check more easily if it's an uniform scale or not
    vec3<F32> _scale;
    ///This is the actual model matrix, but it will not convert to world space as it depends on it's parent in graph
    mat4<F32> _worldMatrix;
    ///The interpolated matrix cache. Represents an intermediate matrix between the current matrix and another, external matrix, interpolated by the given factor
    mat4<F32> _worldMatrixInterp;
    ///All orientation/rotation info is stored in a Quaternion (because they are awesome and also have an internal mat4 if needed)
    Quaternion<F32> _orientation;
    ///_dirty is set to true whenever a translation, rotation or scale is applied
    boost::atomic_bool _dirty;
    ///_physicsDirty is a hack flag to sync SGN transform with physics actor's pose
    boost::atomic_bool _physicsDirty;
    ///_rebuildMatrix is set to true only when a rotation or scale is applied to avoid rebuilding matrices on translation only
    boost::atomic_bool _rebuildMatrix;
    Transform*         _parentTransform;
    boost::atomic_bool _hasParentTransform;
    mutable SharedLock _lock;
    mutable SharedLock _parentLock;
};

#endif
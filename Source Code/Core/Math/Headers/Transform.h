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
   
struct TransformValues {
    ///The object's position in the world as a 3 component vector
    vec3<F32> _translation;
    ///Scaling is stored as a 3 component vector. This helps us check more easily if it's an uniform scale or not
    vec3<F32> _scale;
    ///All orientation/rotation info is stored in a Quaternion (because they are awesome and also have an internal mat4 if needed)
    Quaternion<F32> _orientation;
};

class Transform : public GUIDWrapper, private boost::noncopyable {
public:

    Transform();
    Transform(const Quaternion<F32>& orientation, 
              const vec3<F32>& translation, 
              const vec3<F32>& scale);

    ~Transform();

    /// Set the local X,Y and Z position
    void setPosition(const vec3<F32>& position) {
        WriteLock w_lock(this->_lock);
        this->_transformValues._translation.set(position);
        setDirty();
    }

    /// Set the local X,Y and Z scale factors
    void setScale(const vec3<F32>& scale) { 
        WriteLock w_lock(this->_lock); 
        this->_transformValues._scale.set(scale); 
        setDirty();
        rebuildMatrix(); 
    }

    /// Set the local orientation using the Axis-Angle system. The angle can be in either degrees(default) or radians
    void setRotation(const vec3<F32>& axis, F32 degrees, bool inDegrees = true) { 
        setRotation(Quaternion<F32>(axis, degrees, inDegrees));
    }

    /// Set the local orientation using the Euler system. The angles can be in either degrees(default) or radians
    void setRotation(const vec3<F32>& euler, bool inDegrees = true)  { 
        setRotation(Quaternion<F32>(euler.pitch, euler.yaw, euler.roll, inDegrees));
    }

    /// Set the local orientation so that it matches the specified quaternion.
    void setRotation(const Quaternion<F32>& quat) {
        WriteLock w_lock(this->_lock); 
        this->_transformValues._orientation.set(quat);
        this->_transformValues._orientation.normalize();
        setDirty(); 
        rebuildMatrix();
    }

    /// Add the specified translation factors to the current local position
    void translate(const vec3<F32>& axisFactors) { 
        WriteLock w_lock(this->_lock); 
        this->_transformValues._translation += axisFactors;
        setDirty();
    }

    /// Add the specified scale factors to the current local position
    void scale(const vec3<F32>& axisFactors) { 
        WriteLock w_lock(this->_lock); 
        this->_transformValues._scale += axisFactors; 
        setDirty();
        rebuildMatrix(); 
    }

    /// Apply the specified Axis-Angle rotation starting from the current orientation. The angles can be in either degrees(default) or radians
    void rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees = true) { 
        rotate(Quaternion<F32>(axis, degrees, inDegrees));
    }

    /// Apply the specified Euler rotation starting from the current orientation. The angles can be in either degrees(default) or radians
    void rotate(const vec3<F32>& euler, bool inDegrees = true) { 
        rotate(Quaternion<F32>(euler.pitch, euler.yaw, euler.roll, inDegrees));
    }

    /// Apply the specified Quaternion rotation starting from the current orientation.
    void rotate(const Quaternion<F32>& quat) {
        setRotation(this->_transformValues._orientation * quat);
    }

    /// Perform a SLERP rotation towards the specified quaternion
    void rotateSlerp(const Quaternion<F32>& quat, const D32 deltaTime) {
        WriteLock w_lock(this->_lock); 
        this->_transformValues._orientation.slerp(quat, deltaTime); 
        this->_transformValues._orientation.normalize();
        setDirty(); 
        rebuildMatrix();
    }

    /// If any transform value changed (either locally or in the parent transform), return true
    inline bool isDirty() const {
        if (hasParentTransform()) {
            return this->_dirty || this->_parentTransform->isDirty();
        }

        return  this->_dirty;
    }

    /// If a non-uniform scaling factor is currently set (either locally or in the parent transform), return false
    inline bool isUniformScaled() const {
        if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            return getLocalScale().isUniform() && this->_parentTransform->isUniformScaled();
        }

        return getLocalScale().isUniform();
    }


    /// Transformation helper functions. These just call the normal translate/rotate/scale functions
    /// Set an uniform scale on all three axis
    inline void setScale(const F32 scale) {
        this->setScale(vec3<F32>(scale,scale,scale)); 
    }
    /// Set the scaling factor on the X axis
    inline void setScaleX(const F32 scale) {
        this->setScale(vec3<F32>(scale,
                                 this->_transformValues._scale.y,
                                 this->_transformValues._scale.z));
    }
    /// Set the scaling factor on the Y axis
    inline void setScaleY(const F32 scale) {
        this->setScale(vec3<F32>(this->_transformValues._scale.x,
                                 scale,
                                 this->_transformValues._scale.z));
    }
    /// Set the scaling factor on the Z axis
    inline void setScaleZ(const F32 scale) {
        this->setScale(vec3<F32>(this->_transformValues._scale.x,
                                 this->_transformValues._scale.y,
                                 scale));
    }
    /// Increase the scaling factor on all three axis by an uniform factor
    inline void scale(const F32 scale) {
        this->scale(vec3<F32>(scale, scale, scale));
    }
    /// Increase the scaling factor on the X axis by the specified factor
    inline void scaleX(const F32 scale) {
        this->scale(vec3<F32>(scale,
                              this->_transformValues._scale.y,
                              this->_transformValues._scale.z));
    }
    /// Increase the scaling factor on the Y axis by the specified factor
    inline void scaleY(const F32 scale) {
        this->scale(vec3<F32>(this->_transformValues._scale.x,
                              scale,
                              this->_transformValues._scale.z));
    }
    /// Increase the scaling factor on the Z axis by the specified factor
    inline void scaleZ(const F32 scale) {
        this->scale(vec3<F32>(this->_transformValues._scale.x,
                              this->_transformValues._scale.y,
                              scale));
    }
    /// Rotate on the X axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void rotateX(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(1,0,0), angle, inDegrees);
    }
    /// Rotate on the Y axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void rotateY(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(0,1,0), angle, inDegrees);
    }
    /// Rotate on the Z axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void rotateZ(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(0,0,1), angle, inDegrees);
    }
    /// Set the rotation on the X axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void setRotationX(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(1,0,0), angle, inDegrees);
    }
    /// Set the rotation on the X axis (Euler used) by the specified angle (either degrees or radians)
    inline void setRotationXEuler(const F32 angle, bool inDegrees = true) {
        this->setRotation(angle, inDegrees);
    }
    /// Set the rotation on the Y axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void setRotationY(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(0,1,0), angle, inDegrees);
    }
    /// Set the rotation on the Z axis (Axis-Angle used) by the specified angle (either degrees or radians)
    inline void setRotationZ(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(0,0,1), angle, inDegrees);
    }
    /// Translate the object on the X axis by the specified amount
    inline void translateX(const F32 positionX) {
        this->translate(vec3<F32>(positionX, 0.0f, 0.0f));
    }
    /// Translate the object on the Y axis by the specified amount
    inline void translateY(const F32 positionY) {
        this->translate(vec3<F32>(0.0f, positionY, 0.0f));
    }
    /// Translate the object on the Z axis by the specified amount
    inline void translateZ(const F32 positionZ) {
        this->translate(vec3<F32>(0.0f, 0.0f, positionZ));
    }
    /// Set the object's position on the X axis
    inline void setPositionX(const F32 positionX) {
        this->setPosition(vec3<F32>(positionX, 
                                    this->_transformValues._translation.y, 
                                    this->_transformValues._translation.z));
    }
    /// Set the object's position on the Y axis
    inline void setPositionY(const F32 positionY) {
        this->setPosition(vec3<F32>(this->_transformValues._translation.x, 
                                    positionY,
                                    this->_transformValues._translation.z));
    }
    /// Set the object's position on the Z axis
    inline void setPositionZ(const F32 positionZ) {
        this->setPosition(vec3<F32>(this->_transformValues._translation.x,
                                    this->_transformValues._translation.y, 
                                    positionZ));
    }
    /// Return the local scale factor
    inline const vec3<F32>& getLocalScale() const {
        ReadLock r_lock(this->_lock); 
        return this->_transformValues._scale; 
    }
    /// Return the local position
    inline const vec3<F32>& getLocalPosition() const {
        ReadLock r_lock(this->_lock); 
        return this->_transformValues._translation; 
    }
    /// Return the local orientation quaternion
    inline const Quaternion<F32>& getLocalOrientation() const {
        ReadLock r_lock(this->_lock);
        return this->_transformValues._orientation; 
    }
    /// Return the global scale factor (entire hierarchy scale applied recursively)
    inline vec3<F32> getScale() const {
        if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            vec3<F32> parentScale = this->_parentTransform->getScale();
            r_lock.unlock();
            return  parentScale * getLocalScale();
        }

        return getLocalScale();
    }
    /// Return the global position (entire hierarchy position applied recursively)
    inline vec3<F32> getPosition() const {
        if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            vec3<F32> parentPos = this->_parentTransform->getPosition();
            r_lock.unlock();
            return parentPos + getLocalPosition();
        }

        return getLocalPosition();
    }
    /// Return the global orientation quaternion (entire hierarchy orientation applied recursively)
    inline Quaternion<F32> getOrientation() const {
         if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            Quaternion<F32> parentOrientation = this->_parentTransform->getOrientation();
            r_lock.unlock();
            return parentOrientation.inverse() * getLocalOrientation();
        }

        return getLocalOrientation();
    }
    /// Get the local transformation matrix
    inline const mat4<F32>&  getMatrix() { 
        return this->applyTransforms(); 
    }
    /// Get the parent's transformation matrix (if it exists)
    inline mat4<F32> getParentMatrix() const {
        if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            return this->_parentTransform->getGlobalMatrix();
        }
        // Identity
        return mat4<F32>();
    }
    /// Sets the transform to match a certain transformation matrix.
    /// Scale, orientation and translation are extracted from the specified matrix
    inline void setTransforms(const mat4<F32>& transform) {
        WriteLock w_lock(this->_lock);
        Util::Mat4::decompose(transform, this->_transformValues._scale,
                                         this->_transformValues._orientation,
                                         this->_transformValues._translation);
        setDirty();
    }
    /// Get the parent's global transformation
    inline Transform* const getParentTransform() const {
        if (hasParentTransform()) {
            ReadLock r_lock(this->_parentLock);
            return this->_parentTransform;
        }
        return nullptr;
    }
    /// Set the parent's global transform (the parent's transform with its parent's transform applied and so on)
    inline bool setParentTransform(Transform* transform) {
        WriteLock w_lock(this->_parentLock);

        if (!this->_parentTransform && !transform) {
            return false;
        }
        // Avoid setting the same parent transform twice
        if (this->_parentTransform && transform && 
            this->_parentTransform->getGUID() == transform->getGUID()) {
             return false;
        }

        this->_parentTransform = transform;
        this->_hasParentTransform = (transform != nullptr);
        return true;
    }
    /// Set all of the internal values to match those of the specified transform
    inline void clone(Transform* const transform) {
        WriteLock w_lock(this->_lock);
        this->_transformValues._scale.set(transform->getLocalScale());
        this->_transformValues._translation.set(transform->getLocalPosition());
        this->_transformValues._orientation.set(transform->getLocalOrientation());
        setDirty();
        w_lock.unlock();
        setParentTransform(transform->getParentTransform());
    }
    /// Interpolate the current transform values with the specified ones and return the resulting transformation matrix
    mat4<F32> interpolate(const TransformValues& prevTransforms, const D32 factor);
    /// Extract the 3 transform values (position, scale, rotation) from the current instance
    void getValues(TransformValues& transforms) const;
    /// Creates the local transformation matrix using the position, scale and position values
    const mat4<F32>& applyTransforms();
    /// Compares 2 transforms
    bool compare(const Transform* const t);
    ///Reset transform to identity
    void identity();

private:
    inline void clean() {
        this->_dirty = false;
        this->_rebuildMatrix = false; 
    }

    inline void setDirty() {
        this->_dirty = true;
    }

    inline void rebuildMatrix() {
        this->_rebuildMatrix = true;
    }

    inline bool hasParentTransform() const {
        assert(this->_hasParentTransform == (this->_parentTransform != nullptr));
        return this->_hasParentTransform;
    }
    ///Get the absolute transformation matrix. The local matrix with the parent's transforms applied
    inline mat4<F32>  getGlobalMatrix()  { 
        return this->getMatrix() * this->getParentMatrix(); 
    }

private:
    ///The actual scale, rotation and translation values
    TransformValues _transformValues;
    ///This is the actual model matrix, but it will not convert to world space as it depends on it's parent in graph
    mat4<F32> _worldMatrix, _worldMatrixInterp;
    ///_dirty is set to true whenever a translation, rotation or scale is applied
    boost::atomic_bool _dirty;
    ///_rebuildMatrix is set to true only when a rotation or scale is applied to avoid rebuilding matrices on translation only
    boost::atomic_bool _rebuildMatrix;
    Transform*         _parentTransform;
    boost::atomic_bool _hasParentTransform;
    mutable SharedLock _lock;
    mutable SharedLock _parentLock;
};

#endif
/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "Quaternion.h"

#include "Utility/Headers/GUIDWrapper.h"
#include "Platform/Threading/Headers/SharedMutex.h"

namespace Divide {

struct TransformValues {
    /// The object's position in the world as a 3 component vector
    vec3<F32> _translation;
    /// Scaling is stored as a 3 component vector.
    /// This helps us check more easily if it's an uniform scale or not
    vec3<F32> _scale;
    /// All orientation/rotation info is stored in a Quaternion
    /// (because they are awesome and also have an internal mat4 if needed)
    Quaternion<F32> _orientation;

    void operator=(const TransformValues& other) {
        _translation.set(other._translation);
        _scale.set(other._scale);
        _orientation.set(other._orientation);
    }

    bool operator==(const TransformValues& other) const {
        return _scale.compare(other._scale) &&
               _orientation.compare(other._orientation) &&
               _translation.compare(other._translation);
    }
    bool operator!=(const TransformValues& other) const {
        return !_scale.compare(other._scale) ||
               !_orientation.compare(other._orientation) ||
               !_translation.compare(other._translation);
    }
};

class Transform : public GUIDWrapper, private NonCopyable {
   public:
    Transform();
    Transform(const Quaternion<F32>& orientation, const vec3<F32>& translation,
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

    /// Set the local orientation using the Axis-Angle system.
    /// The angle can be in either degrees(default) or radians
    void setRotation(const vec3<F32>& axis, F32 degrees,
                     bool inDegrees = true) {
        setRotation(Quaternion<F32>(axis, degrees, inDegrees));
    }

    /// Set the local orientation using the Euler system.
    /// The angles can be in either degrees(default) or radians
    void setRotation(const vec3<F32>& euler, bool inDegrees = true) {
        setRotation(
            Quaternion<F32>(euler.pitch, euler.yaw, euler.roll, inDegrees));
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

    /// Apply the specified Axis-Angle rotation starting from the current
    /// orientation.
    /// The angles can be in either degrees(default) or radians
    void rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees = true) {
        rotate(Quaternion<F32>(axis, degrees, inDegrees));
    }

    /// Apply the specified Euler rotation starting from the current
    /// orientation.
    /// The angles can be in either degrees(default) or radians
    void rotate(const vec3<F32>& euler, bool inDegrees = true) {
        rotate(Quaternion<F32>(euler.pitch, euler.yaw, euler.roll, inDegrees));
    }

    /// Apply the specified Quaternion rotation starting from the current
    /// orientation.
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

    /// If a non-uniform scaling factor is currently set (either locally or in
    /// the parent transform),
    // return false
    inline bool isUniformScaled() const { return getScale().isUniform(); }

    /// Transformation helper functions. These just call the normal
    /// translate/rotate/scale functions
    /// Set an uniform scale on all three axis
    inline void setScale(const F32 scale) {
        this->setScale(vec3<F32>(scale));
    }
    /// Set the scaling factor on the X axis
    inline void setScaleX(const F32 scale) {
        this->setScale(vec3<F32>(scale, this->_transformValues._scale.y,
                                 this->_transformValues._scale.z));
    }
    /// Set the scaling factor on the Y axis
    inline void setScaleY(const F32 scale) {
        this->setScale(vec3<F32>(this->_transformValues._scale.x, scale,
                                 this->_transformValues._scale.z));
    }
    /// Set the scaling factor on the Z axis
    inline void setScaleZ(const F32 scale) {
        this->setScale(vec3<F32>(this->_transformValues._scale.x,
                                 this->_transformValues._scale.y, scale));
    }
    /// Increase the scaling factor on all three axis by an uniform factor
    inline void scale(const F32 scale) {
        this->scale(vec3<F32>(scale, scale, scale));
    }
    /// Increase the scaling factor on the X axis by the specified factor
    inline void scaleX(const F32 scale) {
        this->scale(vec3<F32>(scale, this->_transformValues._scale.y,
                              this->_transformValues._scale.z));
    }
    /// Increase the scaling factor on the Y axis by the specified factor
    inline void scaleY(const F32 scale) {
        this->scale(vec3<F32>(this->_transformValues._scale.x, scale,
                              this->_transformValues._scale.z));
    }
    /// Increase the scaling factor on the Z axis by the specified factor
    inline void scaleZ(const F32 scale) {
        this->scale(vec3<F32>(this->_transformValues._scale.x,
                              this->_transformValues._scale.y, scale));
    }
    /// Rotate on the X axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    inline void rotateX(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(1, 0, 0), angle, inDegrees);
    }
    /// Rotate on the Y axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    inline void rotateY(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(0, 1, 0), angle, inDegrees);
    }
    /// Rotate on the Z axis (Axis-Angle used) by the specified angle (either
    /// degrees or radians)
    inline void rotateZ(const F32 angle, bool inDegrees = true) {
        this->rotate(vec3<F32>(0, 0, 1), angle, inDegrees);
    }
    /// Set the rotation on the X axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    inline void setRotationX(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(1, 0, 0), angle, inDegrees);
    }
    /// Set the rotation on the X axis (Euler used) by the specified angle
    /// (either degrees or radians)
    inline void setRotationXEuler(const F32 angle, bool inDegrees = true) {
        this->setRotation(angle, inDegrees);
    }
    /// Set the rotation on the Y axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    inline void setRotationY(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(0, 1, 0), angle, inDegrees);
    }
    /// Set the rotation on the Z axis (Axis-Angle used) by the specified angle
    /// (either degrees or radians)
    inline void setRotationZ(const F32 angle, bool inDegrees = true) {
        this->setRotation(vec3<F32>(0, 0, 1), angle, inDegrees);
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
    /// Return the scale factor
    inline const vec3<F32>& getScale() const {
        ReadLock r_lock(this->_lock);
        return this->_transformValues._scale;
    }
    /// Return the position
    inline const vec3<F32>& getPosition() const {
        ReadLock r_lock(this->_lock);
        return this->_transformValues._translation;
    }
    /// Return the orientation quaternion
    inline const Quaternion<F32>& getOrientation() const {
        ReadLock r_lock(this->_lock);
        return this->_transformValues._orientation;
    }
    /// Get the local transformation matrix
    const mat4<F32>& getMatrix();
    /// Sets the transform to match a certain transformation matrix.
    /// Scale, orientation and translation are extracted from the specified
    /// matrix
    inline void setTransforms(const mat4<F32>& transform) {
        WriteLock w_lock(this->_lock);
        Quaternion<F32>& rotation = this->_transformValues._orientation;
        vec3<F32>& position = this->_transformValues._translation;
        vec3<F32>& scale = this->_transformValues._scale;
        // extract translation
        position.set(transform.m[0][3], transform.m[1][3], transform.m[2][3]);

        // extract the rows of the matrix
        vec3<F32> vRows[3] = {
            vec3<F32>(transform.m[0][0], transform.m[1][0], transform.m[2][0]),
            vec3<F32>(transform.m[0][1], transform.m[1][1], transform.m[2][1]),
            vec3<F32>(transform.m[0][2], transform.m[1][2], transform.m[2][2])};

        // extract the scaling factors
        scale.set(vRows[0].length(), vRows[1].length(), vRows[2].length());

        // and the sign of the scaling
        if (transform.det() < 0) {
            scale.set(-scale);
        }

        // and remove all scaling from the matrix
        if (!IS_ZERO(scale.x)) {
            vRows[0] /= scale.x;
        }
        if (!IS_ZERO(scale.y)) {
            vRows[1] /= scale.y;
        }
        if (!IS_ZERO(scale.z)) {
            vRows[2] /= scale.z;
        }

        // build a 3x3 rotation matrix and generate the rotation quaternion from
        // it
        rotation = Quaternion<F32>(mat3<F32>(
            vRows[0].x, vRows[1].x, vRows[2].x, vRows[0].y, vRows[1].y,
            vRows[2].y, vRows[0].z, vRows[1].z, vRows[2].z));
        setDirty();
    }
    /// Set all of the internal values to match those of the specified transform
    inline void clone(Transform* const transform) {
        this->setValues(transform->getValues());
    }
    /// Extract the 3 transform values (position, scale, rotation) from the
    /// current instance
    inline const TransformValues& getValues() const { return _transformValues; }
    /// Set position, scale and rotation based on the specified transform values
    inline void setValues(const TransformValues& values) {
        WriteLock w_lock(this->_lock);
        this->_transformValues._scale.set(values._scale);
        this->_transformValues._translation.set(values._translation);
        this->_transformValues._orientation.set(values._orientation);
        setDirty();
        w_lock.unlock();
    }
    /// Compares 2 transforms
    bool operator==(const Transform& other) const {
        ReadLock r_lock(_lock);
        return _transformValues == other._transformValues;
    }
    bool operator!=(const Transform& other) const {
        ReadLock r_lock(_lock);
        return _transformValues != other._transformValues;
    }
    /// Reset transform to identity
    void identity();

   private:
    inline void setDirty() { this->_dirty = true; }

    inline void rebuildMatrix() { this->_rebuildMatrix = true; }

   private:
    /// The actual scale, rotation and translation values
    TransformValues _transformValues;
    /// This is the actual model matrix, but it will not convert to world space
    /// as it depends on it's parent in graph
    mat4<F32> _worldMatrix;
    /// _dirty is set to true whenever a translation, rotation or scale is
    /// applied
    std::atomic_bool _dirty;
    /// _rebuildMatrix is true when a rotation or scale is applied to avoid
    /// rebuilding matrices on translation
    std::atomic_bool _rebuildMatrix;
    mutable SharedLock _lock;
};

};  // namespace Divide

#endif
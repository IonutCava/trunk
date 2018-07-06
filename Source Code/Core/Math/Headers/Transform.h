/*
   Copyright (c) 2016 DIVIDE-Studio
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

#include "TransformInterface.h"

namespace Divide {

struct TransformValues {
    TransformValues();
    TransformValues(const TransformValues& other);
    /// The object's position in the world as a 3 component vector
    vec3<F32> _translation;
    /// Scaling is stored as a 3 component vector.
    /// This helps us check more easily if it's an uniform scale or not
    vec3<F32> _scale;
    /// All orientation/rotation info is stored in a Quaternion
    /// (because they are awesome and also have an internal mat4 if needed)
    Quaternion<F32> _orientation;

    void operator=(const TransformValues& other);
    bool operator==(const TransformValues& other) const;
    bool operator!=(const TransformValues& other) const;
};

class Transform : public TransformInterface, public GUIDWrapper, private NonCopyable {
   public:
    Transform();
    Transform(const Quaternion<F32>& orientation,
              const vec3<F32>& translation,
              const vec3<F32>& scale);

    ~Transform();

    void setPosition(const vec3<F32>& position) override;
    void setPositionX(const F32 positionX) override;
    void setPositionY(const F32 positionY) override;
    void setPositionZ(const F32 positionZ) override;
    void translate(const vec3<F32>& axisFactors) override;

    void setScale(const vec3<F32>& ammount) override;
    void setScaleX(const F32 ammount) override;
    void setScaleY(const F32 ammount) override;
    void setScaleZ(const F32 ammount) override;
    void scale(const vec3<F32>& axisFactors) override;
    void scaleX(const F32 ammount) override;
    void scaleY(const F32 ammount) override;
    void scaleZ(const F32 ammount) override;

    void setRotation(const vec3<F32>& axis, F32 degrees, bool inDegrees = true) override;
    void setRotation(F32 pitch, F32 yaw, F32 roll, bool inDegrees = true) override;
    void setRotation(const Quaternion<F32>& quat) override;
    void setRotationX(const F32 angle, bool inDegrees = true) override;
    void setRotationY(const F32 angle, bool inDegrees = true) override;
    void setRotationZ(const F32 angle, bool inDegrees = true) override;
    void rotate(const vec3<F32>& axis, F32 degrees, bool inDegrees = true) override;
    void rotate(F32 pitch, F32 yaw, F32 roll, bool inDegrees = true) override;
    void rotate(const Quaternion<F32>& quat) override;
    void rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) override;
    void rotateX(const F32 angle, bool inDegrees = true) override;
    void rotateY(const F32 angle, bool inDegrees = true) override;
    void rotateZ(const F32 angle, bool inDegrees = true) override;

    void getScale(vec3<F32>& scaleOut) const override;
    void getPosition(vec3<F32>& posOut) const override;
    void getOrientation(Quaternion<F32>& quatOut) const override;

    /// Get the local transformation matrix
    /// wasRebuilt is set to true if the matrix was just rebuilt
    const mat4<F32>& getMatrix() override;

    /// Sets the transform to match a certain transformation matrix.
    /// Scale, orientation and translation are extracted from the specified
    /// matrix
    void setTransforms(const mat4<F32>& transform);

    /// Set all of the internal values to match those of the specified transform
    void clone(Transform* const transform);

    /// Extract the 3 transform values (position, scale, rotation) from the
    /// current instance
    const TransformValues& getValues() const;
    /// Set position, scale and rotation based on the specified transform values
    void setValues(const TransformValues& values);

    /// Compares 2 transforms
    bool operator==(const Transform& other) const;
    bool operator!=(const Transform& other) const;

    /// Reset transform to identity
    void identity();

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

#include "Transform.inl"
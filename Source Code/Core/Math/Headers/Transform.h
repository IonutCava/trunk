/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "TransformInterface.h"

namespace Divide {

//Not thread safe!
class Transform final : public ITransform, public GUIDWrapper, NonCopyable {
   public:
    Transform();
    Transform(const Quaternion<F32>& orientation,
              const vec3<F32>& translation,
              const vec3<F32>& scale);

    ~Transform() = default;

    void setPosition(const vec3<F32>& position) final;
    void setPosition(const F32 x, const F32 y, const F32 z) final;
    void setPositionX(const F32 positionX) final;
    void setPositionY(const F32 positionY) final;
    void setPositionZ(const F32 positionZ) final;
    void translate(const vec3<F32>& axisFactors) final;

    void setScale(const vec3<F32>& amount) final;
    void setScaleX(const F32 amount) final;
    void setScaleY(const F32 amount) final;
    void setScaleZ(const F32 amount) final;
    void scale(const vec3<F32>& axisFactors) final;
    void scaleX(const F32 amount) final;
    void scaleY(const F32 amount) final;
    void scaleZ(const F32 amount) final;

    void setRotation(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) final;
    void setRotation(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) final;
    void setRotation(const Quaternion<F32>& quat) final;
    void setRotationX(const Angle::DEGREES<F32> angle) final;
    void setRotationY(const Angle::DEGREES<F32> angle) final;
    void setRotationZ(const Angle::DEGREES<F32> angle) final;
    void rotate(const vec3<F32>& axis, Angle::DEGREES<F32> degrees) final;
    void rotate(Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> roll) final;
    void rotate(const Quaternion<F32>& quat) final;
    void rotateSlerp(const Quaternion<F32>& quat, const D64 deltaTime) final;
    void rotateX(const Angle::DEGREES<F32> angle) final;
    void rotateY(const Angle::DEGREES<F32> angle) final;
    void rotateZ(const Angle::DEGREES<F32> angle) final;

    void getScale(vec3<F32>& scaleOut) const final;
    void getPosition(vec3<F32>& posOut) const final;
    void getOrientation(Quaternion<F32>& quatOut) const final;

    [[nodiscard]] bool isUniformScale() const;

    void getMatrix(mat4<F32>& matrix) final;

    /// Sets the transform to match a certain transformation matrix.
    /// Scale, orientation and translation are extracted from the specified matrix
    void setTransforms(const mat4<F32>& transform);

    /// Set all of the internal values to match those of the specified transform
    void clone(const Transform* const transform);

    /// Extract the 3 transform values (position, scale, rotation) from the current instance
    [[nodiscard]] TransformValues getValues() const final;
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
    /// This is the actual model matrix, but it will not convert to world space as it depends on it's parent in graph
    mat4<F32> _worldMatrix;
    /// _dirty is set to true whenever a translation, rotation or scale is applied
    bool _dirty = false;
    /// _rebuild is true when a rotation or scale is applied to avoid rebuilding matrices on translation
    bool _rebuild = false;
};

};  // namespace Divide

#endif

#include "Transform.inl"
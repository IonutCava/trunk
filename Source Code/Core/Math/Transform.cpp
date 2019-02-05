#include "stdafx.h"

#include "Headers/Transform.h"

namespace Divide {

TransformValues::TransformValues() noexcept
{
}

TransformValues::TransformValues(const TransformValues& other) noexcept
    : _translation(other._translation),
      _scale(other._scale),
      _orientation(other._orientation)
{
}

Transform::Transform()
    : Transform(Quaternion<F32>(), vec3<F32>(0.0f), vec3<F32>(1.0f))
{
}

Transform::Transform(const Quaternion<F32>& orientation,
                     const vec3<F32>& translation, const vec3<F32>& scale)
{
    _notDirty.clear();
    _dontRebuildMatrix.clear();
    _transformValues._scale.set(scale);
    _transformValues._translation.set(translation);
    _transformValues._orientation.set(orientation);
}

Transform::~Transform()
{
}

mat4<F32> Transform::getMatrix() {
    if (!_notDirty.test_and_set()) {
        if (!_dontRebuildMatrix.test_and_set()) {
            // Ordering - a la Ogre:
            _worldMatrix.identity();
            //    1. Scale
            _worldMatrix.setScale(_transformValues._scale);
            //    2. Rotate
            _worldMatrix *= mat4<F32>(GetMatrix(_transformValues._orientation), false);
        }
        //    3. Translate
        _worldMatrix.setTranslation(_transformValues._translation);
    }

    return _worldMatrix;
}

void Transform::setTransforms(const mat4<F32>& transform) {
    _notDirty.clear();
    _dontRebuildMatrix.clear();

    Quaternion<F32>& rotation = _transformValues._orientation;
    vec3<F32>& position = _transformValues._translation;
    vec3<F32>& scale = _transformValues._scale;
    // extract translation
    position.set(transform.m[0][3], transform.m[1][3], transform.m[2][3]);

    // extract the rows of the matrix
    vec3<F32> vRows[3] = {
        vec3<F32>(transform.m[0][0], transform.m[1][0], transform.m[2][0]),
        vec3<F32>(transform.m[0][1], transform.m[1][1], transform.m[2][1]),
        vec3<F32>(transform.m[0][2], transform.m[1][2], transform.m[2][2]) };

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

    // build a 3x3 rotation matrix and generate the rotation quaternion from it
    rotation = Quaternion<F32>(mat3<F32>(vRows[0].x, vRows[1].x, vRows[2].x,
                                         vRows[0].y, vRows[1].y, vRows[2].y,
                                         vRows[0].z, vRows[1].z, vRows[2].z));
}

void Transform::identity() {
    _transformValues._scale.set(1.0f);
    _transformValues._translation.reset();
    _transformValues._orientation.identity();
    _worldMatrix.identity();
    _notDirty.test_and_set();
    _dontRebuildMatrix.test_and_set();
}

};  // namespace Divide
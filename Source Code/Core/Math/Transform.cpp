#include "Headers/Transform.h"

namespace Divide {
Transform::Transform()
    : Transform(Quaternion<F32>(), vec3<F32>(0.0f), vec3<F32>(1.0f))
{
}

Transform::Transform(const Quaternion<F32>& orientation,
                     const vec3<F32>& translation, const vec3<F32>& scale)
    : GUIDWrapper() 
{
    _dirty = true;
    _rebuildMatrix = true;
    _transformValues._scale.set(scale);
    _transformValues._translation.set(translation);
    _transformValues._orientation.set(orientation);
}

Transform::~Transform()
{
}

const mat4<F32>& Transform::getMatrix(bool& wasRebuilt) {
    if (_dirty) {
        WriteLock w_lock(_lock);
        if (_rebuildMatrix) {
            // Ordering - a la Ogre:
            _worldMatrix.identity();
            //    1. Scale
            _worldMatrix.setScale(_transformValues._scale);
            //    2. Rotate
            _worldMatrix *= GetMatrix(_transformValues._orientation);
            _rebuildMatrix = false;
        }
        //    3. Translate
        _worldMatrix.setTranslation(_transformValues._translation);
        _dirty = false;
        wasRebuilt = true;
    }

    return _worldMatrix;
}

void Transform::identity() {
    WriteLock w_lock(_lock);
    _transformValues._scale.set(1.0f);
    _transformValues._translation.reset();
    _transformValues._orientation.identity();
    _worldMatrix.identity();
    _rebuildMatrix = _dirty = false;
}

};  // namespace Divide
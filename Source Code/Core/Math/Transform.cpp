#include "Headers/Transform.h"

Transform::Transform()	: GUIDWrapper(),
                          _dirty(true),
                          _physicsDirty(true),
                          _rebuildMatrix(true),
                          _hasParentTransform(false)

{
    _transformValues._scale.set(1.0f);
    _transformValues._translation.set(0.0f);
    _transformValues._orientation.identity();
    _worldMatrix.identity();
    _parentTransform = nullptr;
}

Transform::Transform(const Quaternion<F32>& orientation,
                     const vec3<F32>& translation,
                     const vec3<F32>& scale) :  GUIDWrapper(),
                                                _dirty(true),
                                                _physicsDirty(true),
                                                _rebuildMatrix(true),
                                                _hasParentTransform(false)/*,
                                                _changedLastFrame(false),
                                                _changedLastFramePrevious(false)*/
{
    _transformValues._scale.set(scale);
    _transformValues._translation.set(translation);
    _transformValues._orientation.set(orientation);
    _worldMatrix.identity();
    _parentTransform = nullptr;
}

Transform::~Transform()
{
}

const mat4<F32>& Transform::applyTransforms(){
    if(_dirty){
        WriteLock w_lock(_lock);
        if(_rebuildMatrix){
            // Ordering - a la Ogre:
            _worldMatrix.identity();
            //    1. Scale
            _worldMatrix.setScale(_transformValues._scale);
            //    2. Rotate
            _worldMatrix *= ::getMatrix(_transformValues._orientation);
        }
        //    3. Translate
        _worldMatrix.setTranslation(_transformValues._translation);

        this->clean();
    }

    return _worldMatrix;
}

mat4<F32> Transform::interpolate(const TransformValues& prevTransforms, const D32 factor){
   if(factor < 0.90){
        _worldMatrixInterp.identity();
        _worldMatrixInterp.setScale(lerp(prevTransforms._scale, getLocalScale(), (F32)factor));
        _worldMatrixInterp *= ::getMatrix(slerp(prevTransforms._orientation, getLocalOrientation(), (F32)factor));
        _worldMatrixInterp.setTranslation(lerp(prevTransforms._translation, getLocalPosition(), (F32)factor));

        return _worldMatrixInterp * getParentMatrix();
    }

    return getGlobalMatrix();
}

void Transform::getValues(TransformValues& transforms) {
    transforms._scale.set(getLocalScale());
    transforms._orientation.set(getLocalOrientation());
    transforms._translation.set(getLocalPosition());
}

bool Transform::compare(const Transform* const t){
    ReadLock r_lock(_lock);
    return (_transformValues._scale.compare(t->_transformValues._scale) &&
            _transformValues._orientation.compare(t->_transformValues._orientation) &&
            _transformValues._translation.compare(t->_transformValues._translation));
}

void Transform::identity() {
    WriteLock w_lock(_lock);
    _transformValues._scale = vec3<F32>(1.0f);
    _transformValues._translation.reset();
    _transformValues._orientation.identity();
    _worldMatrix.identity();
    clean();
}
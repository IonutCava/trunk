#include "Headers/Transform.h"

Transform::Transform()	: GUIDWrapper(),
                          _dirty(true),
                          _physicsDirty(true),
                          _rebuildMatrix(true),
                          _hasParentTransform(false),
                          /*_changedLastFrame(false),
                          /_changedLastFramePrevious(false),*/
                          _scale(vec3<F32>(1.0f)),
                          _translation(vec3<F32>())

{
    _orientation.identity();
    _worldMatrix.identity();
    WriteLock w_lock(_parentLock);
    _parentTransform = nullptr;
}

Transform::Transform(const Quaternion<F32>& orientation,
                     const vec3<F32>& translation,
                     const vec3<F32>& scale) :  GUIDWrapper(),
                                                _orientation(orientation),
                                                _translation(translation),
                                                _scale(scale),
                                                _dirty(true),
                                                _physicsDirty(true),
                                                _rebuildMatrix(true),
                                                _hasParentTransform(false)/*,
                                                _changedLastFrame(false),
                                                _changedLastFramePrevious(false)*/
{
    _worldMatrix.identity();
    WriteLock w_lock(_parentLock);
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
            _worldMatrix.setScale(_scale);
            //    2. Rotate
            _worldMatrix *= _orientation.getMatrix();
        }
        //    3. Translate
        _worldMatrix.setTranslation(_translation);

        this->clean();
    }

    return _worldMatrix;
}

const mat4<F32>& Transform::interpolate(Transform* const transform, const D32 factor){
   if(transform && factor < 0.99){
      /*mat4<F32> prevMatrix = transform->getGlobalMatrix().transpose();
        vec3<F32> scale, translation;
        Quaternion<F32> orientation;
        Util::Mat4::decompose(prevMatrix, scale, orientation, translation);

        scale.lerp(getScale(), factor);
        translation.lerp(getPosition(), factor);
        orientation.slerp(getOrientation(), factor);
    
        _worldMatrixInterp.identity();
        _worldMatrix.setScale(scale);
        _worldMatrixInterp *= orientation.getMatrix();
        _worldMatrixInterp.setTranslation(translation);

        return _worldMatrixInterp;*/
        _worldMatrixInterp.set(getGlobalMatrix());
    }
    _worldMatrixInterp.set(getGlobalMatrix());

    return _worldMatrixInterp;
}

bool Transform::compare(const Transform* const t){
    ReadLock r_lock(_lock);
    return (_scale.compare(t->_scale) &&
            _orientation.compare(t->_orientation) &&
            _translation.compare(t->_translation));
}

void Transform::identity() {
    WriteLock w_lock(_lock);
    _scale = vec3<F32>(1.0f);
    _translation.reset();
    _orientation.identity();
    _worldMatrix.identity();
    clean();
}

void Transform::update(const U64 deltaTime) {
    //_changedLastFramePrevious = (_changedLastFrame == true);
    //_changedLastFrame = false;
}
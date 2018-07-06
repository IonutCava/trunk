#include "Headers/Transform.h"

Transform::Transform()	: _dirty(true),
                          _physicsDirty(true),
                          _rebuildMatrix(true),
                          _hasParentTransform(false),
                          _scale(vec3<F32>(1.0f)),
                          _translation(vec3<F32>()),
                          _parentTransform(NULL)

{
    _orientation.identity();
    _worldMatrix.identity();
}

Transform::Transform(const Quaternion<F32>& orientation,
                     const vec3<F32>& translation,
                     const vec3<F32>& scale) :  _orientation(orientation),
                                                _translation(translation),
                                                _scale(scale),
                                                _dirty(true),
                                                _physicsDirty(true),
                                                _rebuildMatrix(true),
                                                _hasParentTransform(false),
                                                _parentTransform(NULL)
{
    _worldMatrix.identity();
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

    return applyTransforms();
}

bool Transform::compare(const Transform* const t){
    ReadLock r_lock(_lock);
    return (_scale.compare(t->_scale) &&
            _orientation.compare(t->_orientation) &&
            _translation.compare(t->_translation));
}
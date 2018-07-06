#include "Headers/Transform.h"

Transform::Transform()	: _dirty(false), _rebuildMatrix(false) //<no trasforms applied
{
	WriteLock w_lock(_lock);
	_worldMatrix.identity();
	_parentMatrix.identity();
	_scale = vec3<F32>(1,1,1);
}

Transform::Transform(const Quaternion<F32>& orientation, 
					 const vec3<F32>& translation, 
					 const vec3<F32>& scale) :  _orientation(orientation),
												_translation(translation),
												_scale(scale),
												_dirty(true), 
												_rebuildMatrix(true)
{
	WriteLock w_lock(_lock);
	_parentMatrix.identity();
}

Transform::~Transform()
{

}

void Transform::applyTransforms(){
	if(!_dirty) return;
	WriteLock w_lock(_lock);
	if(_rebuildMatrix){
		// Ordering - a la Ogre:
		_worldMatrix.identity();
		//    1. Scale
		_worldMatrix.scale(_scale);
		//    2. Rotate
		_worldMatrix *= _orientation.getMatrix();
	}
	//    3. Translate
	_worldMatrix.translate(_translation);

	this->clean();
}

bool Transform::compare(Transform* t){
    ReadLock r_lock(_lock);
	if(_scale.compare(t->_scale) &&  _orientation.compare(t->_orientation) && _translation.compare(t->_translation))
		   return true;
	return false;
}
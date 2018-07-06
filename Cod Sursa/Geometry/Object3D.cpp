#include "Object3D.h"
#include "Managers/SceneManager.h"

Object3D::Object3D(const Object3D& old) : _color(old._color), _selected(old._selected), _update(old._update),
										  _name(old._name), _itemName(old._itemName), _bb(old._bb), 
										  _originalBB(old._originalBB),_geometryType(old._geometryType),
										  _drawBB(old._drawBB),_render(old._render)
{
	if(old._transform)	_transform = New Transform(*old._transform);
}

void Object3D::onDraw()
{
	setVisibility(SceneManager::getInstance().getActiveScene()->drawObjects());

	if(!_bb.isComputed()) computeBoundingBox();
	_bb.Transform(_originalBB, getTransform()->getMatrix());
	drawBBox();
}

void Object3D::drawBBox()
{
	_bb.setVisibility(SceneManager::getInstance().getActiveScene()->drawBBox());

	if(_bb.getVisibility())
		GFXDevice::getInstance().drawBox3D(_bb.getMin(),_bb.getMax());
}

Transform*	const Object3D::getTransform()  
{
	if(!_transform){
		Quaternion rotation; rotation.FromEuler(0,0,-1);
		_transform = New Transform(rotation, vec3(0,0,0),vec3(1,1,1));
	}

	return _transform;
}
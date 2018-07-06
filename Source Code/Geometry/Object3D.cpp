#include "Object3D.h"

Object3D::Object3D(const Object3D& old) : SceneNode(old), _selected(old._selected), _update(old._update),
										  _geometryType(old._geometryType),
										  _drawBB(old._drawBB),_render(old._render),_computedLightShaders(old._computedLightShaders)
{
}


void Object3D::render(){
	onDraw();
	GFXDevice::getInstance().renderModel(this);
}

void Object3D::onDraw(){
	getMaterial()->computeLightShaders(); //if it was previously computed, it just returns;
	
	if(!_boundingBox.isComputed()) computeBoundingBox();
	if(getTransform()){
		_boundingBox.Transform(getOriginalBoundingBox(), getTransform()->getMatrix());
	}
	drawBBox();
}
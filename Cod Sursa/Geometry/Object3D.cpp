#include "Object3D.h"

void Object3D::updateBBox()
{
	if(!_bb.isComputed()) computeBoundingBox();
	_bb.Transform(_originalBB, getTransform()->getMatrix());
}
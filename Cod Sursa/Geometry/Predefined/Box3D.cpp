#include "Box3D.h"
#include "Hardware/Video/GFXDevice.h"

void Box3D::draw()
{
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(getPosition());
	GFXDevice::getInstance().rotate(getOrientation());
	GFXDevice::getInstance().scale(getScale());
	GFXDevice::getInstance().setColor(getColor().r,getColor().g,getColor().b);
	GFXDevice::getInstance().drawCube(_size);
	GFXDevice::getInstance().popMatrix();
}
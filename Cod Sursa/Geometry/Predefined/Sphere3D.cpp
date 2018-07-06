#include "Sphere3D.h"
#include "Hardware/Video/GFXDevice.h"

void Sphere3D::draw()
{
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(getPosition());
	GFXDevice::getInstance().rotate(getOrientation());
	GFXDevice::getInstance().scale(getScale());
	GFXDevice::getInstance().setColor(getColor().r,getColor().g,getColor().b);
	GFXDevice::getInstance().drawSphere(_size,_resolution);
	GFXDevice::getInstance().popMatrix();
}
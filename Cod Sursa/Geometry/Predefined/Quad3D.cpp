#include "Quad3D.h"
#include "Hardware/Video/GFXDevice.h"

void Quad3D::draw()
{
	GFXDevice::getInstance().pushMatrix();
	GFXDevice::getInstance().translate(getPosition());
	GFXDevice::getInstance().rotate(getOrientation());
	GFXDevice::getInstance().scale(getScale());
	GFXDevice::getInstance().setColor(getColor().r,getColor().g,getColor().b);
	GFXDevice::getInstance().drawQuad(_tl,_tr,_bl,_br);
	GFXDevice::getInstance().popMatrix();
}
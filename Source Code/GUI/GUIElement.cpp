#include "Headers/GUIElement.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Headers/GFXDevice.h"

GUIElement::GUIElement(GUIType type,const vec2<F32>& position) : 
                           _guiType(type),
						   _parent(NULL),
						   _active(false),
						   _position(position){
						   _name = "defaultGuiControl";
						   _visible = true;
	RenderStateBlockDescriptor d;
	d.setCullMode(CULL_MODE_NONE);
	d.setZEnable(false);
	_guiSB = GFX_DEVICE.createStateBlock(d);

}

GUIElement::~GUIElement(){

	SAFE_DELETE(_guiSB);
}
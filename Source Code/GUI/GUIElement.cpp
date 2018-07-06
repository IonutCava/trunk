#include "Headers/GUIElement.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Headers/GFXDevice.h"

GUIElement::GUIElement(CEGUI::Window*const  parent,const GUIType& type,const vec2<I32>& position) :
                           _guiType(type),
						   _parent(parent),
						   _active(false),
						   _position(position){
						   _name = "defaultGuiControl";
						   _visible = true;
	RenderStateBlockDescriptor d;
	d.setCullMode(CULL_MODE_NONE);
	d.setZEnable(false);
	d.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
	_guiSB = GFX_DEVICE.createStateBlock(d);
}

GUIElement::~GUIElement(){
	SAFE_DELETE(_guiSB);
}
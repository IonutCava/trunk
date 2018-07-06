#include "Headers/GUIElement.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Headers/GFXDevice.h"

GUIElement::GUIElement() : _guiType(GUI_PLACEHOLDER),
						   _parent(NULL),
						   _active(false) {
						   _name = "defaultGuiControl";
						   _visible = true;
	RenderStateBlockDescriptor d;
	d.setCullMode(CULL_MODE_None);
	d.setZEnable(false);
	_guiSB = GFX_DEVICE.createStateBlock(d);

}

GUIElement::~GUIElement(){

	SAFE_DELETE(_guiSB);
}
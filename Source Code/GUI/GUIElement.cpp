#include "Headers/GUIElement.h"
#include "Hardware/Video/Headers/RenderStateBlock.h"
#include "Hardware/Video/Headers/GFXDevice.h"

namespace Divide {

GUIElement::GUIElement(CEGUI::Window*const  parent,const GUIType& type,const vec2<I32>& position) :
                           _guiType(type),
                           _parent(parent),
                           _active(false),
                           _position(position),
                            _lastDrawTimer(0)
{
    _name = "defaultGuiControl";
    _visible = true;

    RenderStateBlockDescriptor desc;
    desc.setCullMode(CULL_MODE_NONE);
    desc.setZEnable(false);
    desc.setBlend(true, BLEND_PROPERTY_SRC_ALPHA, BLEND_PROPERTY_INV_SRC_ALPHA);
    _guiSBHash = GFX_DEVICE.getOrCreateStateBlock(desc);
}

GUIElement::~GUIElement()
{
}

};
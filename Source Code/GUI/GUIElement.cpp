#include "Headers/GUIElement.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GUIElement::GUIElement(CEGUI::Window* const parent, const GUIType& type)
    : _guiType(type),
      _parent(parent),
      _active(false),
      _lastDrawTimer(0)
{
    _name = "defaultGuiControl";
    _visible = true;

    RenderStateBlock stateBlock;
    stateBlock.setCullMode(CullMode::NONE);
    stateBlock.setZRead(false);
    stateBlock.setZWrite(false);
    stateBlock.setBlend(true, BlendProperty::SRC_ALPHA,
                        BlendProperty::INV_SRC_ALPHA);
    _guiSBHash = stateBlock.getHash();
}

GUIElement::~GUIElement()
{
}

};
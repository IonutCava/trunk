#include "Headers/GUIElement.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {

GUIElement::GUIElement(ULL ID, CEGUI::Window* const parent, const GUIType& type)
    : GUIDWrapper(),
      _guiType(type),
      _parent(parent),
      _active(false),
      _ID(ID)
{
    _name = "defaultGuiControl";
    _visible = true;

    RenderStateBlock stateBlock;
    stateBlock.setCullMode(CullMode::NONE);
    stateBlock.setZRead(false);
    stateBlock.setBlend(true, BlendProperty::SRC_ALPHA,
                        BlendProperty::INV_SRC_ALPHA);
    _guiSBHash = stateBlock.getHash();
}

GUIElement::~GUIElement()
{
}

};
#include "stdafx.h"

#include "Headers/GUIElement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

GUIElement::GUIElement(U64 guiID, CEGUI::Window* const parent, const GUIType& type)
    : GUIDWrapper(),
      _guiType(type),
      _parent(parent),
      _active(false),
      _guiID(guiID)
{
    _name = "defaultGuiControl";
    _visible = true;
}

GUIElement::~GUIElement()
{
}

};
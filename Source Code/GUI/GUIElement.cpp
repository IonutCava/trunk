#include "stdafx.h"

#include "Headers/GUIElement.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Video/Headers/RenderStateBlock.h"

namespace Divide {

GUIElement::GUIElement(const stringImpl& name, CEGUI::Window* const parent)
    : GUIDWrapper(),
      _parent(parent),
      _name(name)
{
}

};
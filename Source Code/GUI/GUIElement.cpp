#include "stdafx.h"

#include "Headers/GUIElement.h"

namespace Divide {

GUIElement::GUIElement(const stringImpl& name, CEGUI::Window* const parent)
    : GUIDWrapper(),
      _name(name),
      _parent(parent)
{
}

};
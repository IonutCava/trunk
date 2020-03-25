#include "stdafx.h"

#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(const stringImpl& name,
                 const stringImpl& text,
                 bool  multiLine,
                 const RelativePosition2D& relativePosition,
                 const stringImpl& font,
                 const UColour4& colour,
                 CEGUI::Window* parent,
                 U8 fontSize)
    : GUIElementBase(name, parent),
      TextElement(TextLabelStyle(font.c_str(), colour, fontSize), relativePosition)
{
    this->text(text.c_str(), multiLine);
}

const RelativePosition2D& GUIText::getPosition() const {
    return _position;
}
};
#include "stdafx.h"

#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(U64 guiID,
                 const stringImpl& name,
                 const stringImpl& text,
                 const RelativePosition2D& relativePosition,
                 const stringImpl& font,
                 const UColour4& colour,
                 CEGUI::Window* parent,
                 U8 fontSize)
    : GUIElement(guiID, name, parent, GUIType::GUI_TEXT),
      TextElement(TextLabelStyle(font.c_str(), colour, fontSize), relativePosition)
{
    this->text(text);
}

const RelativePosition2D& GUIText::getPosition() const {
    return _position;
}
};
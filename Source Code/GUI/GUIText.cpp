#include "stdafx.h"

#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(U64 guiID,
                 const stringImpl& text,
                 const RelativePosition2D& relativePosition,
                 const stringImpl& font,
                 const vec4<U8>& colour,
                 CEGUI::Window* parent,
                 U8 fontSize)
    : GUIElement(guiID, parent, GUIType::GUI_TEXT),
      TextElement(TextLabelStyle(font.c_str(), colour, fontSize), relativePosition)
{
}

void GUIText::draw(GFXDevice& context, GFX::CommandBuffer& bufferInOut) const {
    if (!text().empty()) {
        Attorney::GFXDeviceGUI::drawText(context, TextElementBatch(*this), bufferInOut);
    }
}

const RelativePosition2D& GUIText::getPosition() const {
    return _position;
}

// Return true if input was consumed
bool GUIText::mouseMoved(const GUIEvent &event) { return false; }

// Return true if input was consumed
bool GUIText::onMouseUp(const GUIEvent &event) { return false; }

// Return true if input was consumed
bool GUIText::onMouseDown(const GUIEvent &event) { return false; }
};
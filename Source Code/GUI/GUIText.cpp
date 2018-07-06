#include "stdafx.h"

#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(U64 guiID,
                 const stringImpl& text,
                 const vec2<F32>& relativePosition,
                 const stringImpl& font,
                 const vec3<F32>& colour,
                 CEGUI::Window* parent,
                 U32 fontSize)
    : GUIElement(guiID, parent, GUIType::GUI_TEXT),
      TextLabel(text, font, colour, fontSize),
      _position(relativePosition)
{
}

void GUIText::draw(GFXDevice& context) const {
    if (!text().empty()) {
        static vectorImpl<GUITextBatchEntry> entry(1);
        entry[0]._textLabel = this;
        entry[0]._position.set(getPosition());
        entry[0]._stateHash = getStateBlockHash();
        Attorney::GFXDeviceGUI::drawText(context, entry);
    }
}

vec2<F32> GUIText::getPosition() const {
    return vec2<F32>(_position.width, _position.height);
}

// Return true if input was consumed
bool GUIText::mouseMoved(const GUIEvent &event) { return false; }

// Return true if input was consumed
bool GUIText::onMouseUp(const GUIEvent &event) { return false; }

// Return true if input was consumed
bool GUIText::onMouseDown(const GUIEvent &event) { return false; }
};
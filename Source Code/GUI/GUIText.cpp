#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(const stringImpl& id, const stringImpl& text,
                 const vec2<I32>& position, const stringImpl& font,
                 const vec3<F32>& color, CEGUI::Window* parent, U32 textHeight)
    : GUIElement(parent, GUIType::GUI_TEXT, position),
      TextLabel(text, font, color, textHeight)
{
}


void GUIText::draw() const {
    Attorney::GFXDeviceGUI::drawText(GFX_DEVICE, *this, getPosition());
}

void GUIText::onResize(const vec2<I32> &newSize) {
    _position.x -= newSize.x;
    _position.y -= newSize.y;
}

void GUIText::mouseMoved(const GUIEvent &event) {}

void GUIText::onMouseUp(const GUIEvent &event) {}

void GUIText::onMouseDown(const GUIEvent &event) {}
};
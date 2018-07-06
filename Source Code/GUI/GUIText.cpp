#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(ULL ID,
                 const stringImpl& text,
                 const vec2<F32>& relativePosition,
                 const stringImpl& font,
                 const vec3<F32>& color,
                 CEGUI::Window* parent,
                 U32 fontSize)
    : GUIElement(ID, parent, GUIType::GUI_TEXT),
      TextLabel(text, font, color, fontSize),
      _heightCache(0.0f),
      _position(relativePosition)
{
}

const bool GUIText::isVisible() const {
    if (!text().empty()) {
        return GUIElement::isVisible();
    }

    return false;
}

void GUIText::draw() const {
    if (!text().empty()) {
        Attorney::GFXDeviceGUI::drawText(*this,
                                         getStateBlockHash(),
                                         vec2<F32>(_position.width, _heightCache - _position.height));
    }
}

void GUIText::onChangeResolution(U16 w, U16 h) {
    _heightCache = h;
}

void GUIText::mouseMoved(const GUIEvent &event) {}

void GUIText::onMouseUp(const GUIEvent &event) {}

void GUIText::onMouseDown(const GUIEvent &event) {}
};
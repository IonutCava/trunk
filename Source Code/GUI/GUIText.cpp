#include "Headers/GUIText.h"
#include "Platform/Video/Headers/GFXDevice.h"

namespace Divide {
GUIText::GUIText(const stringImpl& id,
                 const stringImpl& text,
                 const vec2<F32>& relativePosition,
                 const stringImpl& font,
                 const vec3<F32>& color,
                 CEGUI::Window* parent,
                 U32 textHeight)
    : GUIElement(parent, GUIType::GUI_TEXT),
      TextLabel(text, font, color, textHeight),
      _heightCache(0.0f),
      _position(relativePosition)
{
}


void GUIText::draw() const {
    Attorney::GFXDeviceGUI::drawText(*this,
                                     getStateBlockHash(),
                                     vec2<F32>(_position.width, _heightCache - _position.height));
}

void GUIText::onChangeResolution(U16 w, U16 h) {
    _heightCache = h;
}

void GUIText::mouseMoved(const GUIEvent &event) {}

void GUIText::onMouseUp(const GUIEvent &event) {}

void GUIText::onMouseDown(const GUIEvent &event) {}
};
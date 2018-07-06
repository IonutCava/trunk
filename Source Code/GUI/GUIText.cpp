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
    _relativePosition(relativePosition)
{
}


void GUIText::draw() const {
    Attorney::GFXDeviceGUI::drawText(GFX_DEVICE, 
                                     *this,
                                     getStateBlockHash(),
                                     _relativePosition);
}

void GUIText::mouseMoved(const GUIEvent &event) {}

void GUIText::onMouseUp(const GUIEvent &event) {}

void GUIText::onMouseDown(const GUIEvent &event) {}
};
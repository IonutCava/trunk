#include "Headers/GUIButton.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

namespace Divide {

GUIButton::GUIButton(const stringImpl& id,
                     const stringImpl& text,
                     const stringImpl& guiScheme,
                     const vec2<F32>& relativeOffset,
                     const vec2<F32>& relativeDimensions,
                     const vec3<F32>& color,
                     CEGUI::Window* parent,
                     ButtonCallback callback)
    : GUIElement(parent, GUIType::GUI_BUTTON),
      _text(text),
      _color(color),
      _callbackFunction(callback),
      _highlight(false),
      _pressed(false),
      _btnWindow(nullptr)
{
    
    _btnWindow = CEGUI::WindowManager::getSingleton().createWindow(guiScheme + "/Button", id);
    _btnWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(relativeOffset.x / 100, 0.0f),
                                            CEGUI::UDim(relativeOffset.y / 100, 0.0f)));
    _btnWindow->setSize(CEGUI::USize(CEGUI::UDim(relativeDimensions.x / 100, 0.0f),
                                     CEGUI::UDim(relativeDimensions.y / 100, 0.0f)));
    _btnWindow->setText(text);
    _btnWindow->subscribeEvent(
        CEGUI::PushButton::EventClicked,
        CEGUI::Event::Subscriber(&GUIButton::joystickButtonPressed, this));
    _parent->addChild(_btnWindow);
    _btnWindow->setEnabled(true);
    setActive(true);
}

GUIButton::~GUIButton()
{
    _parent->removeChild(_btnWindow);
}

void GUIButton::draw() const {
    //Nothing. CEGUI should handle this
}

void GUIButton::setTooltip(const stringImpl& tooltipText) {
    _btnWindow->setTooltipText(tooltipText);
}

void GUIButton::setFont(const stringImpl& fontName,
                        const stringImpl& fontFileName, U32 size) {
    if (!fontName.empty()) {
        if (!CEGUI::FontManager::getSingleton().isDefined(fontName)) {
            CEGUI::FontManager::getSingleton().createFreeTypeFont(
                fontName.c_str(), to_float(size), true, fontFileName);
        }

        if (CEGUI::FontManager::getSingleton().isDefined(fontName)) {
            _btnWindow->setFont(fontName);
        }
    }
}

bool GUIButton::joystickButtonPressed(const CEGUI::EventArgs& /*e*/) {
    if (_callbackFunction) {
        _callbackFunction();
        return true;
    }
    return false;
}
};
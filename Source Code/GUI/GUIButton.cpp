#include "Headers/GUIButton.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Resources/Headers/ResourceCache.h"
#include "Platform/Audio/Headers/SFXDevice.h"

#ifndef CEGUI_STATIC
#define CEGUI_STATIC
#include <CEGUI/CEGUI.h>
#endif //CEGUI_STATIC

namespace Divide {

GUIButton::GUIButton(U64 guiID,
                     const stringImpl& text,
                     const stringImpl& guiScheme,
                     const vec2<F32>& relativeOffset,
                     const vec2<F32>& relativeDimensions,
                     CEGUI::Window* parent,
                     ButtonCallback callback)
    : GUIElement(guiID, parent, GUIType::GUI_BUTTON),
      _callbackFunction(callback),
      _btnWindow(nullptr)
{
    
    _btnWindow = CEGUI::WindowManager::getSingleton().createWindow((guiScheme + "/Button").c_str(), text.c_str());
    _btnWindow->setPosition(CEGUI::UVector2(CEGUI::UDim(relativeOffset.x / 100, 0.0f),
                                            CEGUI::UDim(relativeOffset.y / 100, 0.0f)));
    _btnWindow->setSize(CEGUI::USize(CEGUI::UDim(relativeDimensions.x / 100, 0.0f),
                                     CEGUI::UDim(relativeDimensions.y / 100, 0.0f)));
    _btnWindow->setText(text.c_str());
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventClicked,
                               CEGUI::Event::Subscriber(&GUIButton::joystickButtonPressed, this));
    _parent->addChild(_btnWindow);
    _btnWindow->setEnabled(true);

    stringImpl assetPath = ParamHandler::instance().getParam<stringImpl>(_ID("assetsLocation"));

    ResourceDescriptor beepSound("buttonClick");
    beepSound.setResourceLocation(assetPath + "/sounds/beep.wav");
    beepSound.setFlag(false);
    _onClickSound = CreateResource<AudioDescriptor>(beepSound);
    setActive(true);
}

GUIButton::~GUIButton()
{
    _parent->removeChild(_btnWindow);
}

void GUIButton::setActive(const bool active) {
    _btnWindow->setEnabled(active);
    GUIElement::setActive(active);
}

void GUIButton::setVisible(const bool visible) {
    _btnWindow->setVisible(visible);
    GUIElement::setVisible(visible);
}

void GUIButton::setText(const stringImpl& text) {
    _btnWindow->setText(text.c_str());
}

void GUIButton::draw() const {
    //Nothing. CEGUI should handle this
}

void GUIButton::setTooltip(const stringImpl& tooltipText) {
    _btnWindow->setTooltipText(tooltipText.c_str());
}

void GUIButton::setFont(const stringImpl& fontName,
                        const stringImpl& fontFileName, U32 size) {
    if (!fontName.empty()) {
        if (!CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())) {
            CEGUI::FontManager::getSingleton().createFreeTypeFont(
                fontName.c_str(), to_float(size), true, fontFileName.c_str());
        }

        if (CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())) {
            _btnWindow->setFont(fontName.c_str());
        }
    }
}

bool GUIButton::joystickButtonPressed(const CEGUI::EventArgs& /*e*/) {
    if (_callbackFunction) {
        _callbackFunction(getGUID());
        if (_onClickSound) {
            SFXDevice::instance().playSound(_onClickSound);
        }
        return true;
    }
    return false;
}

void GUIButton::setOnClickSound(const AudioDescriptor_ptr& onClickSound) {
    _onClickSound = onClickSound;
}

};
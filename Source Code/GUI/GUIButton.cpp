#include "stdafx.h"

#include "Headers/GUIButton.h"

#include "Platform/Audio/Headers/SFXDevice.h"

#include <CEGUI/CEGUI.h>

namespace Divide {

GUIButton::AudioCallback GUIButton::_soundCallback;

GUIButton::GUIButton(U64 guiID,
                     const stringImpl& text,
                     const stringImpl& guiScheme,
                     const RelativePosition2D& offset,
                     const RelativeScale2D& size,
                     CEGUI::Window* parent,
                     ButtonCallback callback,
                     AudioDescriptor_ptr onClickSound)
    : GUIElement(guiID, parent, GUIType::GUI_BUTTON),
      _callbackFunction(callback),
      _btnWindow(nullptr),
      _onClickSound(onClickSound)
{
    
    _btnWindow = CEGUI::WindowManager::getSingleton().createWindow((guiScheme + "/Button").c_str(), text.c_str());

    _btnWindow->setPosition(offset);

    _btnWindow->setSize(size);

    _btnWindow->setText(text.c_str());

    _btnWindow->subscribeEvent(CEGUI::PushButton::EventClicked,
                               CEGUI::Event::Subscriber(&GUIButton::joystickButtonPressed, this));

    _parent->addChild(_btnWindow);
    setActive(true);
}

GUIButton::~GUIButton()
{
    _parent->removeChild(_btnWindow);
}

void GUIButton::setActive(const bool active) {
    if (isActive() != active) {
        _btnWindow->setEnabled(active);
        GUIElement::setActive(active);
    }
}

void GUIButton::setVisible(const bool visible) {
    if (isVisible() != visible) {
        _btnWindow->setVisible(visible);
        GUIElement::setVisible(visible);
    }
}

void GUIButton::setText(const stringImpl& text) {
    _btnWindow->setText(text.c_str());
}

void GUIButton::setTooltip(const stringImpl& tooltipText) {
    _btnWindow->setTooltipText(tooltipText.c_str());
}

void GUIButton::setFont(const stringImpl& fontName,
                        const stringImpl& fontFileName, U32 size) {
    if (!fontName.empty()) {
        if (!CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())) {
            CEGUI::FontManager::getSingleton().createFreeTypeFont(
                fontName.c_str(), to_F32(size), true, fontFileName.c_str());
        }

        if (CEGUI::FontManager::getSingleton().isDefined(fontName.c_str())) {
            _btnWindow->setFont(fontName.c_str());
        }
    }
}

bool GUIButton::joystickButtonPressed(const CEGUI::EventArgs& /*e*/) {
    if (_callbackFunction) {
        _callbackFunction(getGUID());
        if (_onClickSound && _soundCallback) {
            _soundCallback(_onClickSound);
        }
        return true;
    }
    return false;
}

void GUIButton::setOnClickSound(const AudioDescriptor_ptr& onClickSound) {
    _onClickSound = onClickSound;
}


bool GUIButton::soundCallback(const AudioCallback& cbk) {
    bool hasCbk = _soundCallback ? true : false;
    _soundCallback = cbk;

    return hasCbk;
}

};
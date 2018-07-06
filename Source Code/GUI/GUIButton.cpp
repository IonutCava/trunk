#include "stdafx.h"

#include "Headers/GUIButton.h"

#include "Platform/Audio/Headers/SFXDevice.h"

#include <CEGUI/CEGUI.h>

namespace Divide {


GUIButton::AudioCallback GUIButton::s_soundCallback;

GUIButton::GUIButton(U64 guiID,
                     const stringImpl& name,
                     const stringImpl& text,
                     const stringImpl& guiScheme,
                     const RelativePosition2D& offset,
                     const RelativeScale2D& size,
                     CEGUI::Window* parent)
    : GUIElement(guiID, name, parent, GUIType::GUI_BUTTON),
      _btnWindow(nullptr)
{
    
    static stringImpl buttonInfo = guiScheme + "/Button";

    _btnWindow = CEGUI::WindowManager::getSingleton().createWindow(buttonInfo.c_str(), name.c_str());

    _btnWindow->setPosition(offset);

    _btnWindow->setSize(size);

    _btnWindow->setText(text.c_str());

    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseMove,
                               CEGUI::Event::Subscriber(&GUIButton::onMove, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseEntersArea,
                               CEGUI::Event::Subscriber(&GUIButton::onHoverEnter, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseLeavesArea,
                               CEGUI::Event::Subscriber(&GUIButton::onHoverLeave, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseButtonDown,
                               CEGUI::Event::Subscriber(&GUIButton::onButtonDown, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseButtonUp,
                               CEGUI::Event::Subscriber(&GUIButton::onButtonUp, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseClick,
                               CEGUI::Event::Subscriber(&GUIButton::onClick, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseDoubleClick,
                               CEGUI::Event::Subscriber(&GUIButton::onDoubleClick, this));
    _btnWindow->subscribeEvent(CEGUI::PushButton::EventMouseDoubleClick,
                               CEGUI::Event::Subscriber(&GUIButton::onTripleClick, this));
    _parent->addChild(_btnWindow);

    setActive(true);
}

GUIButton::~GUIButton()
{
    _btnWindow->removeAllEvents();
    _parent->removeChild(_btnWindow);
}

void GUIButton::setActive(const bool active) {
    if (isActive() != active) {
        GUIElement::setActive(active);
        _btnWindow->setEnabled(active);
    }
}

void GUIButton::setVisible(const bool visible) {
    if (isVisible() != visible) {
        GUIElement::setVisible(visible);
        _btnWindow->setVisible(visible);
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

bool GUIButton::soundCallback(const AudioCallback& cbk) {
    bool hasCbk = s_soundCallback ? true : false;
    s_soundCallback = cbk;

    return hasCbk;
}

bool GUIButton::onEvent(Event event, const CEGUI::EventArgs& /*e*/) {
    if (_callbackFunction[to_base(event)]) {
        _callbackFunction[to_base(event)](getGUID());
        if (_eventSound[to_base(event)] && s_soundCallback) {
            s_soundCallback(_eventSound[to_base(event)]);
        }
        return true;
    }
    return false;
}

bool GUIButton::onMove(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseMove, e);
}

bool GUIButton::onHoverEnter(const CEGUI::EventArgs& e) {
    return onEvent(Event::HoverEnter, e);
}

bool GUIButton::onHoverLeave(const CEGUI::EventArgs& e) {
    return onEvent(Event::HoverLeave, e);
}

bool GUIButton::onButtonDown(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseDown, e);
}

bool GUIButton::onButtonUp(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseUp, e);
}

bool GUIButton::onClick(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseClick, e);
}

bool GUIButton::onDoubleClick(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseDoubleClick, e);
}

bool GUIButton::onTripleClick(const CEGUI::EventArgs& e) {
    return onEvent(Event::MouseTripleClick, e);
}

void GUIButton::setEventCallback(Event event, ButtonCallback callback) {
    _callbackFunction[to_base(event)] = callback;
}

void GUIButton::setEventSound(Event event, AudioDescriptor_ptr sound) {
    _eventSound[to_base(event)] = sound;
}

void GUIButton::setEventCallback(Event event, ButtonCallback callback, AudioDescriptor_ptr sound) {
    setEventCallback(event, callback);
    setEventSound(event, sound);
}

};
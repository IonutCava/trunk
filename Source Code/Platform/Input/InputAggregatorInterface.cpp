#include "stdafx.h"

#include "Headers/InputAggregatorInterface.h"
#include "Core/Math/Headers/MathVectors.h"
#include "Platform/Headers/DisplayWindow.h"
#include "Core/Headers/StringHelper.h"

namespace Divide {
namespace Input {

MouseButton mouseButtonByName(const stringImpl& buttonName) {
    if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Left")) {
        return MouseButton::MB_Left;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Right")) {
        return MouseButton::MB_Right;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Middle")) {
        return MouseButton::MB_Middle;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button3")) {
        return MouseButton::MB_Button3;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button4")) {
        return MouseButton::MB_Button4;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button5")) {
        return MouseButton::MB_Button5;
    } else if (Util::CompareIgnoreCase("MB_" + buttonName, "MB_Button6")) {
        return MouseButton::MB_Button6;
    }

    return MouseButton::MB_Button7;
}

JoystickElement joystickElementByName(const stringImpl& elementName) {
    JoystickElement ret = {};

    if (Util::CompareIgnoreCase(elementName, "POV")) {
        ret._type = JoystickElementType::POV_MOVE;
    } else if (Util::CompareIgnoreCase(elementName, "AXIS")) {
        ret._type = JoystickElementType::AXIS_MOVE;
    } else if (Util::CompareIgnoreCase(elementName, "BALL")) {
        ret._type = JoystickElementType::BALL_MOVE;
    }
    
    if (ret._type != JoystickElementType::COUNT) {
        return ret;
    }

    // Else, we have a button
    ret._type = JoystickElementType::BUTTON_PRESS;

    vector<stringImpl> buttonElements = Util::Split<vector<stringImpl>, stringImpl>(elementName.c_str(), '_');
    assert(buttonElements.size() == 2 && "Invalid joystick element name!");
    assert(Util::CompareIgnoreCase(buttonElements[0], "BUTTON"));
    ret._elementIndex = Util::ConvertData<U32, stringImpl>(buttonElements[1].c_str());

    return ret;
}

InputEvent::InputEvent(DisplayWindow* sourceWindow, U8 deviceIndex) noexcept
    : _deviceIndex(deviceIndex),
      _sourceWindow(sourceWindow)
{
}

MouseButtonEvent::MouseButtonEvent(DisplayWindow* sourceWindow, U8 deviceIndex)
    : InputEvent(sourceWindow, deviceIndex)
{

}

MouseMoveEvent::MouseMoveEvent(DisplayWindow* sourceWindow, U8 deviceIndex, MouseState stateIn, bool wheelEvent)
    : InputEvent(sourceWindow, deviceIndex),
      _stateIn(stateIn),
      _wheelEvent(wheelEvent)
{
}

const MouseState& MouseMoveEvent::state() const noexcept {
    return _stateIn;
}

MouseAxis MouseMoveEvent::X() const {
    return state().X;
}

MouseAxis MouseMoveEvent::Y() const {
    return state().Y;
}

I32 MouseMoveEvent::WheelH() const noexcept {
    return _stateIn.HWheel;
}

I32 MouseMoveEvent::WheelV() const noexcept {
    return _stateIn.VWheel;
}

vec2<I32> MouseMoveEvent::relativePos() const {
    return vec2<I32>(state().X.rel, state().Y.rel);
}

vec2<I32> MouseMoveEvent::absolutePos() const {
    return vec2<I32>(state().X.abs, state().Y.abs);
}

bool MouseMoveEvent::wheelEvent() const noexcept {
    return _wheelEvent;
}

JoystickEvent::JoystickEvent(DisplayWindow* sourceWindow, U8 deviceIndex)
    : InputEvent(sourceWindow, deviceIndex)
{
}

KeyEvent::KeyEvent(DisplayWindow* sourceWindow, U8 deviceIndex)
    : InputEvent(sourceWindow, deviceIndex),
      _key(static_cast<KeyCode>(0)),
      _pressed(false),
      _text(0)
{
}

UTF8Event::UTF8Event(DisplayWindow* sourceWindow, U8 deviceIndex, const char* text)
    : InputEvent(sourceWindow, deviceIndex),
      _text(text)
{

}
}; //namespace Input
}; //namespace Divide
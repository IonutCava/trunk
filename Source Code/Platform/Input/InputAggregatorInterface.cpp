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

InputEvent::InputEvent(DisplayWindow* sourceWindow, U8 deviceIndex)
    : _deviceIndex(deviceIndex),
      _sourceWindow(sourceWindow)
{
}

MouseButtonEvent::MouseButtonEvent(DisplayWindow* sourceWindow, U8 deviceIndex)
    : InputEvent(sourceWindow, deviceIndex)
{

}

MouseMoveEvent::MouseMoveEvent(DisplayWindow* sourceWindow, U8 deviceIndex, MouseState stateIn)
    : InputEvent(sourceWindow, deviceIndex),
      _stateIn(stateIn)
{
}

MouseState MouseMoveEvent::state(bool warped, bool viewportRelative) const {
    auto adjust = [](MouseAxis& inOut, I32 x, I32 y, I32 z, I32 w) -> void {
        D64 slope = 0.0;
        inOut.abs = MAP(inOut.abs, x, y, z, w, slope);
        inOut.rel = static_cast<int>(std::round(inOut.rel * slope));
    };

    MouseState ret = _stateIn;

    if (_sourceWindow->warp() && warped) {
        const Rect<I32>& rect = _sourceWindow->warpRect();
        if (rect.contains(_stateIn.X.abs, _stateIn.Y.abs)) {
            adjust(ret.X, rect.x, rect.z, 0, _sourceWindow->getDimensions().width);
            adjust(ret.Y, rect.y, rect.w, 0, _sourceWindow->getDimensions().height);
        }
    }
    
    if (viewportRelative) {
        const Rect<I32>& rect = _sourceWindow->renderingViewport();
        adjust(ret.X, 0, _sourceWindow->getDimensions().width, rect.x, rect.z);
        adjust(ret.Y, 0, _sourceWindow->getDimensions().height, rect.y, rect.w);
    }

    return ret;
}

MouseAxis MouseMoveEvent::X(bool warped, bool viewportRelative) const {
    return state(warped, viewportRelative).X;
}

MouseAxis MouseMoveEvent::Y(bool warped, bool viewportRelative) const {
    return state(warped, viewportRelative).Y;
}

I32 MouseMoveEvent::WheelH() const {
    return _stateIn.HWheel;
}

I32 MouseMoveEvent::WheelV() const {
    return _stateIn.VWheel;
}

vec4<I32> MouseMoveEvent::relativePos(bool warped, bool viewportRelative) const {
    MouseState crtState = state(warped, viewportRelative);

    return vec4<I32>(crtState.X.rel, crtState.Y.rel, crtState.VWheel, crtState.HWheel);
}

vec4<I32> MouseMoveEvent::absolutePos(bool warped, bool viewportRelative) const {
    MouseState crtState = state(warped, viewportRelative);

    return vec4<I32>(crtState.X.abs, crtState.Y.abs, crtState.VWheel, crtState.HWheel);
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
#include "stdafx.h"

#include "Headers/InputAggregatorInterface.h"
#include "Core/Math/Headers/MathVectors.h"
#include "Platform/Headers/DisplayWindow.h"

namespace Divide {
namespace Input {

InputEvent::InputEvent(U8 deviceIndex)
    : _deviceIndex(deviceIndex)
{
}

MouseEvent::MouseEvent(U8 deviceIndex, const OIS::MouseEvent& arg, const DisplayWindow& parentWindow)
    : InputEvent(deviceIndex),
      _event(arg),
      _parentWindow(parentWindow)
{
}

MouseState MouseEvent::state(bool warped, bool viewportRelative) const {
    auto adjust = [](OIS::Axis& inOut, I32 x, I32 y, I32 z, I32 w) -> void {
        D64 slope = 0.0;
        inOut.abs = MAP(inOut.abs, x, y, z, w, slope);
        inOut.rel = static_cast<int>(std::round(inOut.rel * slope));
    };

    const OIS::MouseState& stateIn = _event.state;

    MouseState ret{ stateIn.X, stateIn.Y, stateIn.Z };
    ret.Z.rel /= 120; //Magic MS/OIS number

    if (_parentWindow.warp() && warped) {
        const Rect<I32>& rect = _parentWindow.warpRect();
        if (rect.contains(stateIn.X.abs, stateIn.Y.abs)) {
            adjust(ret.X, rect.x, rect.z, 0, _event.state.width);
            adjust(ret.Y, rect.y, rect.w, 0, _event.state.height);
        }
    }
    
    if (viewportRelative) {
        const Rect<I32>& rect = _parentWindow.renderingViewport();
        /*adjust(ret.X, 0, _event.state.width, rect.x, rect.z);
        adjust(ret.Y, 0, _event.state.height, rect.y, rect.w);*/

        adjust(ret.X, rect.x, rect.z, 0, _event.state.width);
        adjust(ret.Y, rect.y, rect.w, 0, _event.state.height);
    }

    return ret;
}

OIS::Axis MouseEvent::X(bool warped, bool viewportRelative) const {
    return state(warped, viewportRelative).X;
}

OIS::Axis MouseEvent::Y(bool warped, bool viewportRelative) const {
    return state(warped, viewportRelative).Y;
}

OIS::Axis MouseEvent::Z(bool warped, bool viewportRelative) const {
    return state(warped, viewportRelative).Z;
}

vec3<I32> MouseEvent::relativePos(bool warped, bool viewportRelative) const {
    MouseState crtState = state(warped, viewportRelative);

    return vec3<I32>(crtState.X.rel, crtState.Y.rel, crtState.Z.rel);
}

vec3<I32> MouseEvent::absolutePos(bool warped, bool viewportRelative) const {
    MouseState crtState = state(warped, viewportRelative);

    return vec3<I32>(crtState.X.abs, crtState.Y.abs, crtState.Z.abs);
}

JoystickData::JoystickData() 
    : _deadZone(0),
      _max(0)
{
}

JoystickData::JoystickData(I32 deadZone, I32 max)
        : _deadZone(deadZone),
          _max(max)
{
}

JoystickEvent::JoystickEvent(U8 deviceIndex, const OIS::JoyStickEvent& arg)
    : InputEvent(deviceIndex),
      _event(arg)
{
}

KeyEvent::KeyEvent(U8 deviceIndex)
    : InputEvent(deviceIndex),
      _key(static_cast<KeyCode>(0)),
      _pressed(false),
      _text(0)
{
}

JoystickElement::JoystickElement(JoystickElementType elementType)
    : JoystickElement(elementType, -1)
{
}

JoystickElement::JoystickElement(JoystickElementType elementType, JoystickButton data)
    : _type(elementType),
      _data(data)
{
}

bool JoystickElement::operator==(const JoystickElement &other) const
{
    return (_type == other._type && _data == other._data);
}

}; //namespace Input
}; //namespace Divide
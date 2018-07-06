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

OIS::Axis MouseEvent::X(bool warped, bool viewportRelative) const {
    D64 slope = 0.0;
    OIS::Axis newX = _event.state.X;
    if (_parentWindow.warp() && warped) {
        const vec4<I32>& rect = _parentWindow.warpRect();
        newX.abs = MAP(newX.abs, rect.x, rect.z, 0, _event.state.width, slope);
        newX.rel = static_cast<int>(std::round(newX.rel * slope));
    }

    if (viewportRelative) {
        const vec4<I32>& rect = _parentWindow.renderingViewport();
        newX.abs = MAP(newX.abs, 0, _event.state.width, rect.x, rect.z, slope);
        newX.rel = static_cast<int>(std::round(newX.rel * slope));
    }

    return newX;
}

OIS::Axis MouseEvent::Y(bool warped, bool viewportRelative) const {
    D64 slope = 0.0;
    OIS::Axis newY = _event.state.Y;

    if (_parentWindow.warp() && warped) {
        const vec4<I32>& rect = _parentWindow.warpRect();
        newY.abs = MAP(newY.abs, rect.y, rect.w, 0, _event.state.height, slope);
        newY.rel = static_cast<int>(std::round(newY.rel * slope));
    }

    if (viewportRelative) {
        const vec4<I32>& rect = _parentWindow.renderingViewport();
        newY.abs = MAP(newY.abs, 0, _event.state.height, rect.y, rect.w, slope);
        newY.rel = static_cast<int>(std::round(newY.rel * slope));
    }

    return newY;
}

OIS::Axis MouseEvent::Z(bool warped, bool viewportRelative) const {
    ACKNOWLEDGE_UNUSED(warped);
    ACKNOWLEDGE_UNUSED(viewportRelative);

    return _event.state.Z;
}

vec3<I32> MouseEvent::relativePos(bool warped, bool viewportRelative) const {
    return vec3<I32>(X(warped, viewportRelative).rel, Y(warped, viewportRelative).rel, Z(warped, viewportRelative).rel);
}

vec3<I32> MouseEvent::absolutePos(bool warped, bool viewportRelative) const {
    return vec3<I32>(X(warped, viewportRelative).abs, Y(warped, viewportRelative).abs, Z(warped, viewportRelative).abs);
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
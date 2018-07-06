#include "stdafx.h"

#include "Headers/InputAggregatorInterface.h"
#include "Core/Math/Headers/MathVectors.h"

namespace Divide {
namespace Input {

InputEvent::InputEvent(U8 deviceIndex)
    : _deviceIndex(deviceIndex)
{
}

MouseEvent::MouseEvent(U8 deviceIndex, const OIS::MouseEvent& arg)
    : InputEvent(deviceIndex),
      _event(arg),
      _warp(false),
      _rect(-1)
{
}

OIS::Axis MouseEvent::X(bool warped) const {
    if (_warp && warped) {
        D64 slope = 0.0;
        OIS::Axis newX = _event.state.X;
        newX.abs = MAP(newX.abs, _rect.x, _rect.z, 0, _event.state.width, slope);
        newX.rel = static_cast<int>(std::round(newX.rel * slope));
        return newX;
    }
    return _event.state.X;
}

OIS::Axis MouseEvent::Y(bool warped) const {
    if (_warp && warped) {
        D64 slope = 0.0;
        OIS::Axis newY = _event.state.Y;
        newY.abs = MAP(newY.abs, _rect.y, _rect.w, 0, _event.state.height, slope);
        newY.rel = static_cast<int>(std::round(newY.rel * slope));
        return newY;
    }

    return _event.state.Y;
}

OIS::Axis MouseEvent::Z(bool warped) const {
    /*if (_warp && warped) {
        OIS::Axis newZ = _event.state.Z;
        return newZ;
    }*/
    return _event.state.Z;
}

vec3<I32> MouseEvent::relativePos(bool warped) const {
    return vec3<I32>(X(warped).rel, Y(warped).rel, Z(warped).rel);
}

vec3<I32> MouseEvent::absolutePos(bool warped) const {
    return vec3<I32>(X(warped).abs, Y(warped).abs, Z(warped).abs);
}

void MouseEvent::warp(bool state, const vec4<I32>& rect) {
    _warp = state;
    if (_warp) {
        _rect.set(rect);
    }
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
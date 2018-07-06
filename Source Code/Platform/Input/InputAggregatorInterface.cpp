#include "stdafx.h"

#include "Headers/InputAggregatorInterface.h"

namespace Divide {
namespace Input {

InputEvent::InputEvent(U8 deviceIndex)
    : _deviceIndex(deviceIndex)
{
}

MouseEvent::MouseEvent(U8 deviceIndex, const OIS::MouseEvent& arg)
    : InputEvent(deviceIndex),
      _event(arg)
{
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
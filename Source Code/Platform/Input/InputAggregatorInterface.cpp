#include "Headers/InputAggregatorInterface.h"

namespace Divide {
namespace Input {
JoystickData::JoystickData() : _deadZone(0),
                               _max(0)
{
}

JoystickData::JoystickData(I32 deadZone, I32 max)
        : _deadZone(deadZone),
          _max(max)
{
}

KeyEvent::KeyEvent()
    : _key(static_cast<KeyCode>(0)),
      _pressed(false),
      _text(0)
{
}

JoystickElement::JoystickElement(JoystickElementType elementType)
    : JoystickElement(elementType, -1)
{
}

JoystickElement::JoystickElement(JoystickElementType elementType, I8 data)
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
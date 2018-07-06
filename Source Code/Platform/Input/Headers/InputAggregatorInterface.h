/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _INPUT_AGGREGATOR_INIT_H_
#define _INPUT_AGGREGATOR_INIT_H_

#include "Platform/Headers/PlatformDataTypes.h"
#include <OIS.h>

namespace Divide {
namespace Input {
/// Points to the position of said joystick in the vector
enum class Joystick : U32 {
    JOYSTICK_1 = 0,
    JOYSTICK_2 = 1,
    JOYSTICK_3 = 2,
    JOYSTICK_4 = 3,
    COUNT
};

struct JoystickData {
    JoystickData();
    JoystickData(I32 deadZone, I32 max);

    I32 _deadZone;
    I32 _max;
};

typedef OIS::KeyCode KeyCode;
typedef OIS::Keyboard::Modifier KeyModifier;

typedef OIS::MouseEvent MouseEvent;
typedef OIS::MouseButtonID MouseButton;

typedef OIS::JoyStickEvent JoystickEvent;
typedef I8 JoystickButton;

static const U32 KeyCode_PLACEHOLDER = 0xEE;

struct KeyEvent {
    KeyEvent();

    KeyCode _key;
    bool _pressed;
    U32 _text;
};

enum class JoystickElementType : U32 {
    POV_MOVE = 0,
    AXIS_MOVE,
    SLIDER_MOVE,
    VECTOR_MOVE,
    BUTTON_PRESS,
    COUNT
};

struct JoystickElement {
    JoystickElement(JoystickElementType elementType);
    JoystickElement(JoystickElementType elementType, I8 data);

    bool operator==(const JoystickElement &other) const;

    JoystickElementType _type;
    I8 _data; //< e.g. button index

};

enum class InputState : U32 {
    PRESSED = 0,
    RELEASED,
    COUNT
};

class InputAggregatorInterface {
   public:
    /// Keyboard
    virtual bool onKeyDown(const KeyEvent &arg) = 0;
    virtual bool onKeyUp(const KeyEvent &arg) = 0;
    /// Joystick or Gamepad
    virtual bool joystickButtonPressed(const JoystickEvent &arg, JoystickButton button) = 0;
    virtual bool joystickButtonReleased(const JoystickEvent &arg,
                                        JoystickButton button) = 0;
    virtual bool joystickAxisMoved(const JoystickEvent &arg, I8 axis) = 0;
    virtual bool joystickPovMoved(const JoystickEvent &arg, I8 pov) = 0;
    virtual bool joystickSliderMoved(const JoystickEvent &, I8 index) = 0;
    virtual bool joystickVector3DMoved(const JoystickEvent &arg, I8 index) = 0;
    /// Mouse
    virtual bool mouseMoved(const MouseEvent &arg) = 0;
    virtual bool mouseButtonPressed(const MouseEvent &arg, MouseButton id) = 0;
    virtual bool mouseButtonReleased(const MouseEvent &arg, MouseButton id) = 0;
};

};  // namespace Input
};  // namespace Divide

#endif

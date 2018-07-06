/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _INPUT_AGGREGATOR_INIT_H_
#define _INPUT_AGGREGATOR_INIT_H_

#include "Core/Math/Headers/MathVectors.h"
#include <OIS.h>

namespace Divide {

class DisplayWindow;
namespace Input {
/// Points to the position of said joystick in the vector
enum class Joystick : U8 {
    JOYSTICK_1  = 0,
    JOYSTICK_2  = 1,
    JOYSTICK_3  = 2,
    JOYSTICK_4  = 3,
    JOYSTICK_5  = 4,
    JOYSTICK_6  = 5,
    JOYSTICK_7  = 6,
    JOYSTICK_8  = 7,
    JOYSTICK_9  = 8,
    JOYSTICK_10 = 9,
    COUNT
};

struct MouseState {
    OIS::Axis X, Y, Z;
};

struct JoystickData {
    JoystickData();
    JoystickData(I32 deadZone, I32 max);

    I32 _deadZone;
    I32 _max;
};

typedef OIS::KeyCode KeyCode;
typedef OIS::Keyboard::Modifier KeyModifier;

typedef OIS::MouseButtonID MouseButton;

struct InputEvent {
    explicit InputEvent(U8 deviceIndex);

    U8 _deviceIndex;
};

struct MouseEvent : public InputEvent {
    explicit MouseEvent(U8 deviceIndex, const OIS::MouseEvent& arg, const DisplayWindow& parentWindow);

    OIS::Axis X(bool warped = true, bool viewportRelative = false) const;
    OIS::Axis Y(bool warped = true, bool viewportRelative = false) const;
    OIS::Axis Z(bool warped = true, bool viewportRelative = false) const;


    vec3<I32> relativePos(bool warped = true, bool viewportRelative = false) const;
    vec3<I32> absolutePos(bool warped = true, bool viewportRelative = false) const;
    MouseState state(bool warped = true, bool viewportRelative = false) const;

 private:

    const OIS::MouseEvent& _event;
    const DisplayWindow& _parentWindow;
};

struct JoystickEvent : public InputEvent {
    explicit JoystickEvent(U8 deviceIndex, const OIS::JoyStickEvent& arg);

    const OIS::JoyStickEvent& _event;
};

typedef int JoystickButton;

static const U32 KeyCode_PLACEHOLDER = 0xEE;

struct KeyEvent : public InputEvent {
    explicit KeyEvent(U8 deviceIndex);

    KeyCode _key;
    bool _pressed;
    U32 _text;
};

enum class JoystickElementType : U8 {
    POV_MOVE = 0,
    AXIS_MOVE,
    SLIDER_MOVE,
    VECTOR_MOVE,
    BUTTON_PRESS,
    COUNT
};

struct JoystickElement {
    JoystickElement(JoystickElementType elementType);
    JoystickElement(JoystickElementType elementType, JoystickButton data);

    bool operator==(const JoystickElement &other) const;

    JoystickElementType _type;
    JoystickButton _data; //< e.g. button index
};

enum class InputState : U8 {
    PRESSED = 0,
    RELEASED,
    COUNT
};

class InputAggregatorInterface {
   public:
    /// Keyboard: return true if input was consumed
    virtual bool onKeyDown(const KeyEvent &arg) = 0;
    virtual bool onKeyUp(const KeyEvent &arg) = 0;
    /// Mouse: return true if input was consumed
    virtual bool mouseMoved(const MouseEvent &arg) = 0;
    virtual bool mouseButtonPressed(const MouseEvent &arg, MouseButton id) = 0;
    virtual bool mouseButtonReleased(const MouseEvent &arg, MouseButton id) = 0;

    /// Joystick or Gamepad: return true if input was consumed
    virtual bool joystickButtonPressed(const JoystickEvent &arg, JoystickButton button) = 0;
    virtual bool joystickButtonReleased(const JoystickEvent &arg, JoystickButton button) = 0;
    virtual bool joystickAxisMoved(const JoystickEvent &arg, I8 axis) = 0;
    virtual bool joystickPovMoved(const JoystickEvent &arg, I8 pov) = 0;
    virtual bool joystickSliderMoved(const JoystickEvent &arg, I8 index) = 0;
    virtual bool joystickvector3Moved(const JoystickEvent &arg, I8 index) = 0;
};

};  // namespace Input
};  // namespace Divide

#endif

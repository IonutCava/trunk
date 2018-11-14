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

#include "Input.h"

namespace Divide {

class DisplayWindow;
namespace Input {

struct InputEvent {
    explicit InputEvent(DisplayWindow* sourceWindow, U8 deviceIndex);

    U8 _deviceIndex = 0;
    DisplayWindow* _sourceWindow = nullptr;
};

struct MouseButtonEvent : public InputEvent {
    explicit MouseButtonEvent(DisplayWindow* sourceWindow, U8 deviceIndex);

    bool pressed = false;
    MouseButton button = MouseButton::MB_Left;
    U8 numCliks = 0;
    vec2<I32> relPosition = vec2<I32>(-1);
};

struct MouseMoveEvent : public InputEvent {
    explicit MouseMoveEvent(DisplayWindow* sourceWindow, U8 deviceIndex, MouseState stateIn, bool wheelEvent);

    MouseAxis X() const;
    MouseAxis Y() const;

    I32 WheelV() const;
    I32 WheelH() const;

    vec4<I32> relativePos() const;
    vec4<I32> absolutePos() const;
    const MouseState& state() const;
    
    bool wheelEvent() const;
 private:
    MouseState _stateIn;
    const bool _wheelEvent = false;
};

struct JoystickEvent : public InputEvent {
    explicit JoystickEvent(DisplayWindow* sourceWindow, U8 deviceIndex);

    JoystickElement _element;
};

struct UTF8Event : public InputEvent {
    explicit UTF8Event(DisplayWindow* sourceWindow, U8 deviceIndex, const char* text);

    const char* _text = nullptr;
};

struct KeyEvent : public InputEvent {

    explicit KeyEvent(DisplayWindow* sourceWindow, U8 deviceIndex);

    KeyCode _key = KeyCode::KC_UNASSIGNED;
    bool _pressed = false;
    bool _isRepeat = false;
    const char* _text = nullptr;
    U16 _modMask = 0;
};

class InputAggregatorInterface {
   public:
    /// Keyboard: return true if input was consumed
    virtual bool onKeyDown(const KeyEvent &arg) = 0;
    virtual bool onKeyUp(const KeyEvent &arg) = 0;
    /// Mouse: return true if input was consumed
    virtual bool mouseMoved(const MouseMoveEvent &arg) = 0;
    virtual bool mouseButtonPressed(const MouseButtonEvent& arg) = 0;
    virtual bool mouseButtonReleased(const MouseButtonEvent& arg) = 0;

    /// Joystick or Gamepad: return true if input was consumed
    virtual bool joystickButtonPressed(const JoystickEvent &arg) = 0;
    virtual bool joystickButtonReleased(const JoystickEvent &arg) = 0;
    virtual bool joystickAxisMoved(const JoystickEvent &arg) = 0;
    virtual bool joystickPovMoved(const JoystickEvent &arg) = 0;
    virtual bool joystickBallMoved(const JoystickEvent &arg) = 0;
    virtual bool joystickAddRemove(const JoystickEvent &arg) = 0;
    virtual bool joystickRemap(const JoystickEvent &arg) = 0;

    virtual bool onUTF8(const UTF8Event& arg) = 0;
};

};  // namespace Input
};  // namespace Divide

#endif

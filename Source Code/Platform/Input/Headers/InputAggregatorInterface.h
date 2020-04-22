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
#pragma once
#ifndef _INPUT_AGGREGATOR_INIT_H_
#define _INPUT_AGGREGATOR_INIT_H_

#include "Input.h"

namespace Divide {

class DisplayWindow;
namespace Input {

namespace Attorney {
    class MouseEventKernel;
};

struct InputEvent {
    explicit InputEvent(DisplayWindow* sourceWindow, U8 deviceIndex) noexcept;

    U8 _deviceIndex = 0;
    DisplayWindow* _sourceWindow = nullptr;
};

struct MouseButtonEvent : public InputEvent {
    friend class Attorney::MouseEventKernel;

    explicit MouseButtonEvent(DisplayWindow* sourceWindow, U8 deviceIndex);

    PROPERTY_RW(bool, pressed, false);
    PROPERTY_RW(MouseButton, button, MouseButton::MB_Left);
    PROPERTY_RW(U8, numCliks, 0u);
    PROPERTY_RW(vec2<I32>, absPosition, vec2<I32>(-1));

protected:
    bool _remaped = false;
};

struct MouseMoveEvent : public InputEvent {
    friend class Attorney::MouseEventKernel;

    explicit MouseMoveEvent(DisplayWindow* sourceWindow, U8 deviceIndex, MouseState stateIn, bool wheelEvent);

    MouseAxis X() const noexcept;
    MouseAxis Y() const noexcept;

    I32 WheelV() const noexcept;
    I32 WheelH() const noexcept;

    vec2<I32> relativePos() const noexcept;
    vec2<I32> absolutePos() const noexcept;
    bool remaped() const noexcept;

    const MouseState& state() const noexcept;
    
    bool wheelEvent() const noexcept;

protected:
    void absolutePos(const vec2<I32>& newPos) noexcept;

 private:
    MouseState _stateIn;
    bool _remaped = false;
    const bool _wheelEvent = false;
};
namespace Attorney {
    class MouseEventKernel {
        private:
            static void absolutePos(MouseMoveEvent& evt, const vec2<I32>& newPos) noexcept {
                evt.absolutePos(newPos);
                evt._remaped = true;
            }

            static void absolutePos(MouseButtonEvent& evt, const vec2<I32>& pos) noexcept {
                evt.absPosition(pos);
                evt._remaped = true;
            }
        friend class Kernel;
    };
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

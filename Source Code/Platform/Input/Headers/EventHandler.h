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
#ifndef _INPUT_EVENT_HANDLER_H_
#define _INPUT_EVENT_HANDLER_H_

#include "InputAggregatorInterface.h"

namespace Divide {

class Kernel;

namespace Input {

class EventHandler : public InputAggregatorInterface {
   protected:
    InputAggregatorInterface& _eventListener;

   public:
    explicit EventHandler(InputAggregatorInterface& eventListener);

    /// Key pressed - Engine: return true if input was consumed
    bool onKeyDown(const KeyEvent& arg) override;
    /// Key released - Engine: return true if input was consumed
    bool onKeyUp(const KeyEvent& arg) override;
    /// Joystick button pressed - Engine: return true if input was consumed
    bool joystickButtonPressed(const JoystickEvent& arg) override;
    /// Joystick button released - Engine: return true if input was consumed
    bool joystickButtonReleased(const JoystickEvent& arg) override;
    /// Joystick axis change - Engine: return true if input was consumed
    bool joystickAxisMoved(const JoystickEvent& arg) override;
    /// Joystick direction change - Engine: return true if input was consumed
    bool joystickPovMoved(const JoystickEvent& arg) override;
    /// Joystick ball change - Engine: return true if input was consumed
    bool joystickBallMoved(const JoystickEvent& arg) override;
    /// Joystick was added or removed - Engine: return true if event was consumed
    bool joystickAddRemove(const JoystickEvent& arg) override;
    /// Joystick was remaped. - Engine: return true if event was consumed
    bool joystickRemap(const JoystickEvent &arg) override;

    /// Mouse moved - Engine: return true if input was consumed
    bool mouseMoved(const MouseMoveEvent& arg) override;
    /// Mouse button pressed - Engine: return true if input was consumed
    bool mouseButtonPressed(const MouseButtonEvent& arg) override;
    /// Mouse button released - Engine: return true if input was consumed
    bool mouseButtonReleased(const MouseButtonEvent& arg) override;
    /// UTF8 text input event - Engine: return true if input was consumed
    bool onUTF8(const Input::UTF8Event& arg) override;
};

};  // namespace Input
};  // namespace Divide;
#endif
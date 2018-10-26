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
//'index' and 'arg' in OISJoyStick.h and 'n' in OISMultiTouch.h are unreferenced
#include "InputAggregatorInterface.h"

namespace Divide {

class Kernel;

namespace Input {

class InputInterface;
class JoystickInterface;
class EffectManager;

class EventHandler : public OIS::KeyListener,
                     public OIS::JoyStickListener,
                     public OIS::MouseListener,
                     public InputAggregatorInterface {
   protected:
    InputInterface* _pApplication;
    JoystickInterface* _pJoystickInterface;
    EffectManager* _pEffectMgr;
    InputAggregatorInterface& _eventListener;

   public:
    EventHandler(InputInterface* pApp, InputAggregatorInterface& eventListener);
    void initialize(JoystickInterface* pJoystickInterface,
                    EffectManager* pEffectMgr);

   protected:
    /// Key pressed - Engine: return true if input was consumed
    bool onKeyDown(const KeyEvent& arg);
    /// Key pressed - OIS
    bool keyPressed(const OIS::KeyEvent& arg) override;
    /// Key released - Engine: return true if input was consumed
    bool onKeyUp(const KeyEvent& arg);
    /// Key released - OIS
    bool keyReleased(const OIS::KeyEvent& arg) override;
    /// Joystick button pressed - Engine: return true if input was consumed
    bool joystickButtonPressed(const JoystickEvent& arg, JoystickButton button);
    /// Joystick button pressed - OIS
    bool buttonPressed(const OIS::JoyStickEvent& arg, JoystickButton button) override;
    /// Joystick button released - Engine: return true if input was consumed
    bool joystickButtonReleased(const JoystickEvent& arg, JoystickButton button);
    /// Joystick button released - OIS
    bool buttonReleased(const OIS::JoyStickEvent& arg, JoystickButton button) override;
    /// Joystick axis change - Engine: return true if input was consumed
    bool joystickAxisMoved(const JoystickEvent& arg, I8 axis);
    /// Joystick axis change - OIS
    bool axisMoved(const OIS::JoyStickEvent& arg, int axis) override;
    /// Joystick direction change - Engine: return true if input was consumed
    bool joystickPovMoved(const JoystickEvent& arg, I8 pov);
    /// Joystick direction change - OIS
    bool povMoved(const OIS::JoyStickEvent& arg, int pov) override;
    /// Joystick slider change - Engine: return true if input was consumed
    bool joystickSliderMoved(const JoystickEvent& arg, I8 index);
    /// Joystick slider change - OIS
    bool sliderMoved(const OIS::JoyStickEvent& arg, int index) override;
    /// Joystick 3D vector changed - Engine: return true if input was consumed
    bool joystickvector3Moved(const JoystickEvent& arg, I8 index);
    /// Joystick 3D vector changed - OIS
    bool vector3Moved(const OIS::JoyStickEvent& arg, int index) override;
    /// Mouse moved - OIS
    bool mouseMoved(const OIS::MouseEvent& arg) override;
    /// Mouse moved - Engine: return true if input was consumed
    bool mouseMoved(const MouseEvent& arg);
    /// Mouse button pressed - Engine: return true if input was consumed
    bool mouseButtonPressed(const MouseEvent& arg, MouseButton button);
    /// Mouse button pressed - OIS
    bool mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;
    /// Mouse button released - Engine: return true if input was consumed
    bool mouseButtonReleased(const MouseEvent& arg, MouseButton button);
    /// Mouse button released - OIS
    bool mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id) override;
    bool onSDLInputEvent(SDL_Event event) override;
};
};  // namespace Input
};  // namespace Divide;
#endif
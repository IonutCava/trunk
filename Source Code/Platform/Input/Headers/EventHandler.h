/*
   Copyright (c) 2017 DIVIDE-Studio
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
    Kernel* _kernel;

   public:
    EventHandler(InputInterface* pApp, Kernel& kernel);
    void initialize(JoystickInterface* pJoystickInterface,
                    EffectManager* pEffectMgr);

   protected:
    /// Key pressed - Engine
    bool onKeyDown(const KeyEvent& arg);
    /// Key pressed - OIS
    bool keyPressed(const OIS::KeyEvent& arg);
    /// Key released - Engine
    bool onKeyUp(const KeyEvent& arg);
    /// Key released - OIS
    bool keyReleased(const OIS::KeyEvent& arg);
    /// Joystick button pressed - Engine
    bool joystickButtonPressed(const JoystickEvent& arg,
                               JoystickButton button);
    /// Joystick button pressed - OIS
    bool buttonPressed(const OIS::JoyStickEvent& arg,
                       JoystickButton button);
    /// Joystick button released - Engine
    bool joystickButtonReleased(const JoystickEvent& arg,
                                JoystickButton button);
    /// Joystick button released - OIS
    bool buttonReleased(const OIS::JoyStickEvent& arg,
                        JoystickButton button);
    /// Joystick axis change - Engine
    bool joystickAxisMoved(const JoystickEvent& arg, I8 axis);
    /// Joystick axis change - OIS
    bool axisMoved(const OIS::JoyStickEvent& arg, I8 axis);
    /// Joystick direction change - Engine
    bool joystickPovMoved(const JoystickEvent& arg, I8 pov);
    /// Joystick direction change - OIS
    bool povMoved(const OIS::JoyStickEvent& arg, I8 pov);
    /// Joystick slider change - Engine
    bool joystickSliderMoved(const JoystickEvent& arg, I8 index);
    /// Joystick slider change - OIS
    bool sliderMoved(const OIS::JoyStickEvent& arg, I8 index);
    /// Joystick 3D vector changed - Engine
    bool joystickVector3DMoved(const JoystickEvent& arg, I8 index);
    /// Joystick 3D vector changed - OIS
    bool vector3DMoved(const OIS::JoyStickEvent& arg, I8 index);
    /// Mouse moved - OIS
    bool mouseMoved(const OIS::MouseEvent& arg);
    /// Mouse moved - Engine
    bool mouseMoved(const MouseEvent& arg);
    /// Mouse button pressed - Engine
    bool mouseButtonPressed(const MouseEvent& arg, MouseButton button);
    /// Mouse button pressed - OIS
    bool mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id);
    /// Mouse button released - Engine
    bool mouseButtonReleased(const MouseEvent& arg, MouseButton button);
    /// Mouse button released - OIS
    bool mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id);
};
};  // namespace Input
};  // namespace Divide;
#endif
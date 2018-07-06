/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _INPUT_EVENT_HANDLER_H_
#define _INPUT_EVENT_HANDLER_H_
//'index' and 'arg' in OISJoyStick.h and 'n' in OISMultiTouch.h are unreferenced
#include <OIS.h>

#include "Hardware/Platform/Headers/SharedMutex.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

class InputInterface;
class JoystickInterface;
class EffectManager;
class Kernel;

class EventHandler : public OIS::KeyListener, public OIS::JoyStickListener, public OIS::MouseListener {
  protected:

    InputInterface*    _pApplication;
    JoystickInterface* _pJoystickInterface;
    EffectManager*	   _pEffectMgr;
    Kernel*			   _kernel;

  public:

    EventHandler(InputInterface* pApp, Kernel* const kernel);
    void initialize(JoystickInterface* pJoystickInterface, EffectManager* pEffectMgr);

    ///Keyboard
    bool keyPressed( const OIS::KeyEvent &arg );
    bool keyReleased( const OIS::KeyEvent &arg );
    ///Joystick or Gamepad
    bool buttonPressed( const OIS::JoyStickEvent &arg, I8 button);
    bool buttonReleased( const OIS::JoyStickEvent &arg, I8 button);
    bool axisMoved( const OIS::JoyStickEvent &arg, I8 axis);
    bool povMoved( const OIS::JoyStickEvent &arg, I8 pov);
    bool sliderMoved( const OIS::JoyStickEvent &, I8 index);
    bool vector3Moved( const OIS::JoyStickEvent &arg, I8 index);
    ///Mouse
    bool mouseMoved( const OIS::MouseEvent &arg );
    bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
    bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
};

#endif
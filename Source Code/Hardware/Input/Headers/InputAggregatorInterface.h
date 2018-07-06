/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _INPUT_AGGREGATOR_INIT_H_
#define _INPUT_AGGREGATOR_INIT_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include <OIS.h>

namespace Divide {
    namespace Input {
    ///Points to the position of said joystick in the vector
    enum Joystick {
        JOYSTICK_1 = 0,
        JOYSTICK_2,
        JOYSTICK_3,
        JOYSTICK_4,
        JOYSTICK_5,
        JOYSTICK_6,
        JOYSTICK_7,
        JOYSTICK_8
    };

    typedef OIS::KeyCode            KeyCode;
    typedef OIS::Keyboard::Modifier KeyModifier;
    
    typedef OIS::MouseEvent    MouseEvent;
    typedef OIS::MouseButtonID MouseButton;

    typedef OIS::JoyStickEvent JoystickEvent;
    
    static const U32 KeyCode_PLACEHOLDER = 0xEE;

    struct KeyEvent {
        KeyCode _key;
        bool _pressed;
        U32  _text;

        KeyEvent() : _key(static_cast<KeyCode>(0)),
                     _pressed(false),
                     _text(0)
        {
        }
    };

    class InputAggregatorInterface {

    public :
        ///Keyboard
        virtual bool onKeyDown( const KeyEvent& arg ) = 0;
        virtual bool onKeyUp( const KeyEvent& arg ) = 0;
        ///Joystick or Gamepad
        virtual bool joystickButtonPressed( const JoystickEvent &arg, I8 button) = 0;
        virtual bool joystickButtonReleased( const JoystickEvent &arg, I8 button) = 0;
        virtual bool joystickAxisMoved( const JoystickEvent &arg, I8 axis) = 0;
        virtual bool joystickPovMoved( const JoystickEvent &arg, I8 pov) = 0;
        virtual bool joystickSliderMoved( const JoystickEvent &, I8 index) = 0;
        virtual bool joystickVector3DMoved( const JoystickEvent &arg, I8 index) = 0;
        ///Mouse
        virtual bool mouseMoved( const MouseEvent &arg ) = 0;
        virtual bool mouseButtonPressed( const MouseEvent &arg, MouseButton id ) = 0;
        virtual bool mouseButtonReleased( const MouseEvent &arg, MouseButton id ) = 0;
    };

    }; //namespace Input
}; //namespace Divide
#endif
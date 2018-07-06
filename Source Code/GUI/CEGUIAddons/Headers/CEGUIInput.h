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

#ifndef _CEGUI_INPUT_H_
#define _CEGUI_INPUT_H_
#include "Hardware/Input/Headers/AutoKeyRepeat.h"
#include "Hardware/Input/Headers/InputAggregatorInterface.h"

///This class defines AutoRepeatKey::repeatKey(...) as CEGUI key inputs
class CEGUIInput : public InputAggregatorInterface, public AutoRepeatKey {
public:
    ///Key pressed
    bool onKeyDown(const OIS::KeyEvent& key);
    ///Key released
    bool onKeyUp(const OIS::KeyEvent& key);
    ///Joystick axis change
    bool joystickAxisMoved(const OIS::JoyStickEvent& arg,I8 axis);
    ///Joystick direction change
    bool joystickPovMoved(const OIS::JoyStickEvent& arg,I8 pov);
    ///Joystick button pressed
    bool joystickButtonPressed(const OIS::JoyStickEvent& arg,I8 button);
    ///Joystick button released
    bool joystickButtonReleased(const OIS::JoyStickEvent& arg, I8 button);
    bool joystickSliderMoved( const OIS::JoyStickEvent &arg, I8 index);
    bool joystickVector3DMoved( const OIS::JoyStickEvent &arg, I8 index);
    ///Mouse moved
    bool mouseMoved(const OIS::MouseEvent& arg);
    ///Mouse button pressed
    bool mouseButtonPressed(const OIS::MouseEvent& arg,OIS::MouseButtonID button);
    ///Mouse button released
    bool mouseButtonReleased(const OIS::MouseEvent& arg,OIS::MouseButtonID button);

protected:
    ///Called on key events
    bool injectOISKey(bool pressed,const OIS::KeyEvent& inKey);
   void repeatKey(I32 inKey, U32 Char);
};

#endif
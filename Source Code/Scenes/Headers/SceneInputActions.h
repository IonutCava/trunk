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

#ifndef _SCENE_INPUT_ACTIONS_H_
#define _SCENE_INPUT_ACTIONS_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

struct PressReleaseActions {
    PressReleaseActions();
    PressReleaseActions(U16 onPressAction,
                        U16 onReleaseAction);
    PressReleaseActions(U16 onPressAction,
                        U16 onReleaseAction,
                        U16 onLCtrlPressAction,
                        U16 onLCtrlReleaseAction = 0u,
                        U16 onRCtrlPressAction = 0u,
                        U16 onRCtrlReleaseAction = 0u,
                        U16 onLAltPressAction = 0u,
                        U16 onLAltReleaseAction = 0u,
                        U16 onRAltPressAction = 0u,
                        U16 onRAltReleaseAction = 0u);

    // key only
    U16 _onPressAction;
    U16 _onReleaseAction;

    U16 _onLCtrlPressAction;
    U16 _onLCtrlReleaseAction;
    U16 _onRCtrlPressAction;
    U16 _onRCtrlReleaseAction;

    U16 _onLAltPressAction;
    U16 _onLAltReleaseAction;
    U16 _onRAltPressAction;
    U16 _onRAltReleaseAction;

    inline void clear() {
        _onPressAction = 
        _onReleaseAction = 
        _onLCtrlPressAction = 
        _onLCtrlReleaseAction = 
        _onRCtrlPressAction = 
        _onRCtrlReleaseAction = 
        _onLAltPressAction = 
        _onLAltReleaseAction = 
        _onRAltPressAction = 
        _onRAltReleaseAction = 0;
    }
};

class InputActionList {
   public:
    InputActionList();

    bool registerInputAction(U16 id, DELEGATE_CBK<> action);
    DELEGATE_CBK<> getInputAction(U16 id) const;

   protected:
    hashMapImpl<U16 /*actionID*/, DELEGATE_CBK<> /*action*/> _inputActions;
};

}; //namespace Divide

#endif //_SCENE_INPUT_ACTIONS_H_

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

#ifndef _SCENE_INPUT_ACTIONS_H_
#define _SCENE_INPUT_ACTIONS_H_

#include "Platform/Headers/PlatformDefines.h"

namespace Divide {

struct InputParams {
    InputParams(U8 deviceIndex) noexcept
        : InputParams(deviceIndex, -1)
    {
    }

    InputParams(U8 deviceIndex, I32 var1) noexcept
        : InputParams(deviceIndex, var1, -1)
    {
    }

    InputParams(U8 deviceIndex, I32 var1, I32 var2) noexcept
        : InputParams(deviceIndex, var1, var2, -1)
    {
    }

    InputParams(U8 deviceIndex, I32 var1, I32 var2, I32 var3) noexcept
        : InputParams(deviceIndex, var1, var2, var3, -1)
    {
    }

    InputParams(U8 deviceIndex, I32 var1, I32 var2, I32 var3, I32 var4) noexcept
        : _var{ var1, var2, var3, var4 },
          _deviceIndex(deviceIndex)
    {
    }

    I32 _var[4];
    U8  _deviceIndex;
};

class PressReleaseActions {
public:
    enum class Action : U8 {
        PRESS = 0,
        RELEASE,
        LEFT_CTRL_PRESS,
        LEFT_CTRL_RELEASE,
        RIGHT_CTRL_PRESS,
        RIGHT_CTRL_RELEASE,
        LEFT_ALT_PRESS,
        LEFT_ALT_RELEASE,
        RIGHT_ALT_PRESS,
        RIGHT_ALT_RELEASE,
        COUNT
    };

    inline void clear() {
        for (auto& entries : _actions) {
            entries.resize(0);
        }
    }

    inline bool merge(const PressReleaseActions& other) {
        bool conflict = false;

        for (U8 i = 0; i < to_U8(Action::COUNT); ++i) {
            if (!other._actions[i].empty()) {
                if (!_actions[i].empty()) {
                    conflict = true;
                }
                insert_unique(_actions[i], other._actions[i]);
            }
        }

        return conflict;
    }

    inline const vectorEASTL<U16>& getActionIDs(Action action) const noexcept {
        return _actions[to_base(action)];
    }

    inline void insertActionID(Action action, U16 ID) {
        insert_unique(_actions[to_base(action)], ID);
    }

    inline void insertActionIDs(Action action, const vectorEASTL<U16>& IDs) {
        _actions[to_base(action)] = IDs;
    }

private:
    // keys only
    std::array<vectorEASTL<U16>, to_base(Action::COUNT)> _actions;
};

struct InputAction {
    InputAction(const DELEGATE<void, InputParams>& action);

    DELEGATE<void, InputParams> _action;
    // This will be usefull for menus and the like (defined in XML)
    Str64 _displayName;

    void displayName(const Str64& name);
};

class InputActionList {
   public:
    InputActionList();

    bool registerInputAction(U16 id, const DELEGATE<void, InputParams>& action);
    InputAction& getInputAction(U16 id);
    const InputAction& getInputAction(U16 id) const;

   protected:
    hashMap<U16 /*actionID*/, InputAction> _inputActions;
    InputAction _noOPAction;
};

}; //namespace Divide

#endif //_SCENE_INPUT_ACTIONS_H_

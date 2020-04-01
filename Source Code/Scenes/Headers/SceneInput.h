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

#ifndef _SCENE_INPUT_H_
#define _SCENE_INPUT_H_

#include "SceneInputActions.h"

namespace Divide {
    struct pair_hash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2> &p) const {
            size_t hash = 17;
            Util::Hash_combine(hash, p.first);
            Util::Hash_combine(hash, p.second);

            return hash;
        }
    };
// This is the callback equivalent of PressReleaseAction with IDs resolved
struct PressReleaseActionCbks {
    std::array<vectorSTD<DELEGATE<void, InputParams>>, to_base(PressReleaseActions::Action::COUNT)> _actions;

    void from(const PressReleaseActions& actions, const InputActionList& actionList);
};

class SceneInput : public Input::InputAggregatorInterface {
   public:
    using JoystickMapKey = std::pair<std::underlying_type_t<Input::JoystickElementType>, U32>;

    using KeyMapCache = hashMap<Input::KeyCode, PressReleaseActionCbks>;
    using MouseMapCache = hashMap<Input::MouseButton, PressReleaseActionCbks>;
    using JoystickMapCacheEntry  = ska::bytell_hash_map<JoystickMapKey, PressReleaseActionCbks, pair_hash>;
    using JoystickMapCache = ska::bytell_hash_map<std::underlying_type_t<Input::Joystick>, JoystickMapCacheEntry>;

    using KeyMap = hashMap<Input::KeyCode, PressReleaseActions>;
    using MouseMap = hashMap<Input::MouseButton, PressReleaseActions>;
    using JoystickMapEntry = ska::bytell_hash_map<JoystickMapKey, PressReleaseActions, pair_hash>;
    using JoystickMap = ska::bytell_hash_map<std::underlying_type_t<Input::Joystick>, JoystickMapEntry>;

    using KeyLog = vectorSTD<std::pair<Input::KeyCode, Input::InputState>>;
    using MouseBtnLog = vectorSTD<std::tuple<Input::MouseButton, Input::InputState, vec2<I32>>>;

    explicit SceneInput(Scene &parentScene, PlatformContext& context);

    //Keyboard: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent &arg) override;
    bool onKeyUp(const Input::KeyEvent &arg) override;
    /// Joystick or Gamepad: return true if input was consumed
    bool joystickButtonPressed(const Input::JoystickEvent &arg) override;
    bool joystickButtonReleased(const Input::JoystickEvent &arg) override;
    bool joystickAxisMoved(const Input::JoystickEvent &arg) override;
    bool joystickPovMoved(const Input::JoystickEvent &arg) override;
    bool joystickBallMoved(const Input::JoystickEvent &arg) override;
    bool joystickAddRemove(const Input::JoystickEvent &arg) override;
    bool joystickRemap(const Input::JoystickEvent &arg) override;
    /// Mouse: return true if input was consumed
    bool mouseMoved(const Input::MouseMoveEvent &arg) override;
    bool mouseButtonPressed(const Input::MouseButtonEvent &arg) override;
    bool mouseButtonReleased(const Input::MouseButtonEvent &arg) override;
    bool onUTF8(const Input::UTF8Event& arg) override;
    /// Returns false if the key is already assigned and couldn't be merged
    /// Call removeKeyMapping for the specified key first
    bool addKeyMapping(Input::KeyCode key, PressReleaseActions keyCbks);
    /// Returns false if the key wasn't previously assigned
    bool removeKeyMapping(Input::KeyCode key);
    /// Returns true if the key has a valid mapping and sets the callback output
    /// to the mapping's function
    bool getKeyMapping(Input::KeyCode key, PressReleaseActionCbks& keyCbkOut);

    /// Returns false if the button is already assigned.
    /// Call removeButtonMapping for the specified key first
    bool addMouseMapping(Input::MouseButton button, PressReleaseActions btnCbks);
    /// Returns false if the button wasn't previously assigned
    bool removeMouseMapping(Input::MouseButton button);
    /// Returns true if the button has a valid mapping and sets the callback
    /// output to the mapping's function
    bool getMouseMapping(Input::MouseButton button, PressReleaseActionCbks& btnCbksOut);

    /// Returns false if the button is already assigned.
    /// Call removeJoystickMapping for the specified key first
    bool addJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id, PressReleaseActions btnCbks);
    /// Returns false if the button wasn't previously assigned
    bool removeJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id);
    /// Returns true if the button has a valid mapping and sets the callback
    /// output to the mapping's function
    bool getJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id, PressReleaseActionCbks& btnCbksOut);

    InputActionList& actionList();

    U8 getPlayerIndexForDevice(U8 deviceIndex) const;

    void flushCache();

    void onPlayerAdd(U8 index);
    void onPlayerRemove(U8 index);

    void onSetActive();
    void onRemoveActive();

   protected:
       bool handleCallbacks(const PressReleaseActionCbks& cbks,
                            const InputParams& params,
                            bool onPress);

   private:
    PlatformContext& _context;
    Scene &_parentScene;

    KeyMap _keyMap;
    MouseMap _mouseMap;
    JoystickMap _joystickMap;

    KeyMapCache _keyMapCache;
    MouseMapCache _mouseMapCache;
    JoystickMapCache _joystickMapCache;

    InputActionList _actionList;

    hashMap<U8, KeyLog> _keyLog;
    hashMap<U8, MouseBtnLog> _mouseBtnLog;

};  // SceneInput

};  // namespace Divide
#endif  //_SCENE_INPUT_H_


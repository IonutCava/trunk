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

#ifndef _SCENE_INPUT_H_
#define _SCENE_INPUT_H_

#include "SceneInputActions.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

// This is the callback equivalent of PressReleaseAction with IDs resolved
struct PressReleaseActionCbks {
    // key only
    DELEGATE_CBK<void, InputParams> _onPressAction;
    DELEGATE_CBK<void, InputParams> _onReleaseAction;

    DELEGATE_CBK<void, InputParams> _onLCtrlPressAction;
    DELEGATE_CBK<void, InputParams> _onLCtrlReleaseAction;
    DELEGATE_CBK<void, InputParams> _onRCtrlPressAction;
    DELEGATE_CBK<void, InputParams> _onRCtrlReleaseAction;

    DELEGATE_CBK<void, InputParams> _onLAltPressAction;
    DELEGATE_CBK<void, InputParams> _onLAltReleaseAction;
    DELEGATE_CBK<void, InputParams> _onRAltPressAction;
    DELEGATE_CBK<void, InputParams> _onRAltReleaseAction;

    void from(const PressReleaseActions& actions, const InputActionList& actionList);
};

class JoystickHasher
{
public:
    size_t operator()(const Input::JoystickElement& k) const
    {
        size_t hash = 0;
        Util::Hash_combine(hash, to_U32(k._type));
        Util::Hash_combine(hash, k._data);
        return hash;
    }
};

class SceneInput : public Input::InputAggregatorInterface {
   public:
    typedef hashMapImpl<Input::KeyCode, PressReleaseActionCbks, std::hash<I32>> KeyMapCache;
    typedef hashMapImpl<Input::MouseButton, PressReleaseActionCbks, std::hash<I32>> MouseMapCache;
    typedef hashMapImpl<Input::JoystickElement, PressReleaseActionCbks, JoystickHasher> JoystickMapCacheEntry;
    typedef hashMapImpl<Input::Joystick, JoystickMapCacheEntry> JoystickMapCache;

    typedef hashMapImpl<Input::KeyCode, PressReleaseActions, std::hash<I32>> KeyMap;
    typedef hashMapImpl<Input::MouseButton, PressReleaseActions, std::hash<I32>> MouseMap;
    typedef hashMapImpl<Input::JoystickElement, PressReleaseActions, JoystickHasher> JoystickMapEntry;
    typedef hashMapImpl<Input::Joystick, JoystickMapEntry> JoystickMap;

    typedef vectorImpl<std::pair<Input::KeyCode, Input::InputState>> KeyLog;
    typedef vectorImpl<std::tuple<Input::MouseButton, Input::InputState, vec2<I32>>> MouseBtnLog;

    explicit SceneInput(Scene &parentScene, Input::InputInterface& context);

    //Keyboard: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent &arg);
    bool onKeyUp(const Input::KeyEvent &arg);
    /// Joystick or Gamepad: return true if input was consumed
    bool joystickButtonPressed(const Input::JoystickEvent &arg,
                               Input::JoystickButton button);
    bool joystickButtonReleased(const Input::JoystickEvent &arg,
                                Input::JoystickButton button);
    bool joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis);
    bool joystickPovMoved(const Input::JoystickEvent &arg, I8 pov);
    bool joystickSliderMoved(const Input::JoystickEvent &, I8 index);
    bool joystickVector3DMoved(const Input::JoystickEvent &arg, I8 index);
    /// Mouse: return true if input was consumed
    bool mouseMoved(const Input::MouseEvent &arg);
    bool mouseButtonPressed(const Input::MouseEvent &arg,
                            Input::MouseButton id);
    bool mouseButtonReleased(const Input::MouseEvent &arg,
                             Input::MouseButton id);

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
    bool addJoystickMapping(Input::Joystick device, Input::JoystickElement element, PressReleaseActions btnCbks);
    /// Returns false if the button wasn't previously assigned
    bool removeJoystickMapping(Input::Joystick device, Input::JoystickElement element);
    /// Returns true if the button has a valid mapping and sets the callback
    /// output to the mapping's function
    bool getJoystickMapping(Input::Joystick device, Input::JoystickElement element, PressReleaseActionCbks& btnCbksOut);

    Input::InputState getKeyState(U8 deviceIndex, Input::KeyCode key) const;
    Input::InputState getMouseButtonState(U8 deviceIndex, Input::MouseButton button) const;
    Input::InputState getJoystickButtonState(Input::Joystick deviceIndex, Input::JoystickButton button) const;

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

       bool isDeviceInUse(I32 deviceID);

   private:
    // Up to 2 devices per player: KB + Mouse or Nintendo Switch type controllers
    // If one current player only uses one device, leave pair.second to -1
    hashMapImpl<U8 /*player index*/, std::pair<I32, I32>> _playerControlDevices;

    vectorImpl<I32> _usedInputDevices;

    Input::InputInterface& _context;
    Scene &_parentScene;

    KeyMap _keyMap;
    MouseMap _mouseMap;
    JoystickMap _joystickMap;

    KeyMapCache _keyMapCache;
    MouseMapCache _mouseMapCache;
    JoystickMapCache _joystickMapCache;

    InputActionList _actionList;

    hashMapImpl<U8, KeyLog> _keyLog;
    hashMapImpl<U8, MouseBtnLog> _mouseBtnLog;

};  // SceneInput

};  // namespace Divide
#endif  //_SCENE_INPUT_H_

/*
   Copyright (c) 2015 DIVIDE-Studio
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

#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {
class SceneInput : public Input::InputAggregatorInterface {
   public:
    enum class InputState : U32 { PRESSED = 0, RELEASED = 1, COUNT };

    typedef hashAlg::pair<DELEGATE_CBK<>, DELEGATE_CBK<>> PressReleaseActions;
    typedef hashMapImpl<Input::KeyCode, PressReleaseActions, std::hash<I32>> KeyMap;
    typedef hashMapImpl<Input::MouseButton, PressReleaseActions, std::hash<I32>> MouseMap;
    typedef hashMapImpl<Input::JoystickButton, PressReleaseActions> JoystickMap;

    typedef hashMapImpl<Input::KeyCode, InputState, std::hash<I32>> KeyState;
    typedef hashMapImpl<Input::MouseButton, InputState, std::hash<I32>> MouseBtnState;
    typedef hashMapImpl<Input::JoystickButton, InputState> JoystickBtnState;

    explicit SceneInput(Scene &parentScene);

    inline const vec2<I32>& getMousePosition() const {
        return _mousePos;
    }

    bool onKeyDown(const Input::KeyEvent &arg);
    bool onKeyUp(const Input::KeyEvent &arg);
    /// Joystick or Gamepad
    bool joystickButtonPressed(const Input::JoystickEvent &arg,
                               Input::JoystickButton button);
    bool joystickButtonReleased(const Input::JoystickEvent &arg,
                                Input::JoystickButton button);
    bool joystickAxisMoved(const Input::JoystickEvent &arg, I8 axis);
    bool joystickPovMoved(const Input::JoystickEvent &arg, I8 pov);
    bool joystickSliderMoved(const Input::JoystickEvent &, I8 index);
    bool joystickVector3DMoved(const Input::JoystickEvent &arg, I8 index);
    /// Mouse
    bool mouseMoved(const Input::MouseEvent &arg);
    bool mouseButtonPressed(const Input::MouseEvent &arg,
                            Input::MouseButton id);
    bool mouseButtonReleased(const Input::MouseEvent &arg,
                             Input::MouseButton id);

    /// Returns false if the key is already assigned.
    /// Call removeKeyMapping for the specified key first
    bool addKeyMapping(Input::KeyCode key, const PressReleaseActions &keyCbks);
    /// Returns false if the key wasn't previously assigned
    bool removeKeyMapping(Input::KeyCode key);
    /// Returns true if the key has a valid mapping and sets the callback output
    /// to the mapping's function
    bool getKeyMapping(Input::KeyCode key, PressReleaseActions &keyCbkOut) const;

    /// Returns false if the button is already assigned.
    /// Call removeButtonMapping for the specified key first
    bool addMouseMapping(Input::MouseButton button,
                         const PressReleaseActions &btnCbks);
    /// Returns false if the button wasn't previously assigned
    bool removeMouseMapping(Input::MouseButton button);
    /// Returns true if the button has a valid mapping and sets the callback
    /// output to the mapping's function
    bool getMouseMapping(Input::MouseButton button,
                         PressReleaseActions &btnCbksOut) const;

    /// Returns false if the button is already assigned.
    /// Call removeJoystickMapping for the specified key first
    bool addJoystickMapping(Input::JoystickButton button,
                            const PressReleaseActions &btnCbks);
    /// Returns false if the button wasn't previously assigned
    bool removeJoystickMapping(Input::JoystickButton button);
    /// Returns true if the button has a valid mapping and sets the callback
    /// output to the mapping's function
    bool getJoystickMapping(Input::JoystickButton button,
                            PressReleaseActions &btnCbksOut) const;

    InputState getKeyState(Input::KeyCode key) const;
    InputState getMouseButtonState(Input::MouseButton button) const;
    InputState getJoystickButtonState(Input::JoystickButton button) const;

   private:
    Scene &_parentScene;
    KeyMap _keyMap;
    MouseMap _mouseMap;
    JoystickMap _joystickMap;
    KeyState _keyStateMap;
    MouseBtnState _mouseStateMap;
    JoystickBtnState _joystickStateMap;
    vec2<I32> _mousePos;

    vectorImpl<std::pair<Input::KeyCode, InputState>> _keyLog;
    vectorImpl<std::tuple<Input::MouseButton, InputState, vec2<I32>>> _mouseBtnLog;
};  // SceneInput

};  // namespace Divide
#endif  //_SCENE_INPUT_H_

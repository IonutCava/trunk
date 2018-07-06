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

#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "EffectManager.h"
#include "Core/Headers/Application.h"
#include "Core/Headers/KernelComponent.h"

namespace Divide {
namespace Input {

namespace Attorney {
    class InputInterfaceEvent;
};
//////////// Event handler class declaration ////////////////////////////////////////////////
class InputInterface : public KernelComponent {
    friend class Attorney::InputInterfaceEvent;
public:
    explicit InputInterface(Kernel& parent);
    ~InputInterface();

    ErrorCode init(Kernel& kernel, const vec2<U16>& inputAreaDimensions);

    U8 update(const U64 deltaTime);
    void terminate();

    inline bool isInit() const {
        return _bIsInitialized;
    }

    inline void stop() { _bMustStop = true; }
    inline JoystickInterface* getJoystickInterface() {
        return _pJoystickInterface;
    }
    inline EffectManager& getEffectManager() { return *_pEffectMgr; }
    inline OIS::Keyboard& getKeyboard() const { return *_pKeyboard; }
    inline OIS::Mouse& getMouse() const { return *_pMouse; }

    inline bool isKeyDown(Input::KeyCode keyCode) const {
        if (!_pKeyboard) {
            return false;
        }
        return _pKeyboard->isKeyDown(keyCode);
    }

    inline bool isModifierDown(KeyModifier keyModifier) const {
        if (!_pKeyboard) {
            return false;
        }
        return _pKeyboard->isModifierDown(keyModifier);
    }

    inline const KeyEvent& getKey(KeyCode keyCode) const {
        return _keys[to_uint(keyCode)];
    }

    static KeyCode keyCodeByName(const stringImpl& keyName);
    static MouseButton mouseButtonByName(const stringImpl& buttonName);
    static JoystickElement joystickElementByName(const stringImpl& elementName);

    Joystick joystick(I32 deviceID) const;
    InputState getKeyState(KeyCode keyCode) const;
    InputState getMouseButtonState(MouseButton button) const;
    InputState getJoystickeButtonState(Input::Joystick device, JoystickButton button) const;

protected:
    inline KeyEvent& getKeyRef(U32 index) { return _keys[index]; }

protected:
    OIS::InputManager* _pInputInterface;
    EventHandler* _pEventHdlr;
    OIS::Keyboard* _pKeyboard;
    OIS::Mouse* _pMouse;
    /// multiple joystick support
    vectorImpl<OIS::JoyStick*> _pJoysticks;
    hashMapImpl<I32, Joystick> _joystickIdToEntry;

    JoystickInterface* _pJoystickInterface;
    EffectManager* _pEffectMgr;

    bool _bMustStop;
    bool _bIsInitialized;

    KeyEvent _keys[KeyCode_PLACEHOLDER];

};
namespace Attorney {
    class InputInterfaceEvent {
    private:
        static KeyEvent& getKeyRef(InputInterface& input, U32 index) {
            return input.getKeyRef(index);
        }

        friend class Divide::Input::EventHandler;
    };
};  // namespace Attorney


};  // namespace Input
};  // namespace Divide
#endif

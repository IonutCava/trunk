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

#pragma once
#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "EffectManager.h"
#include "Core/Headers/Application.h"

namespace Divide {
namespace Input {

namespace Attorney {
    class InputInterfaceEvent;
};
//////////// Event handler class declaration ////////////////////////////////////////////////
class InputInterface {
    friend class Attorney::InputInterfaceEvent;
public:
    typedef std::pair<OIS::Keyboard*, OIS::Mouse*> KBMousePair;

public:
    explicit InputInterface(DisplayWindow& parent);
    ~InputInterface();

    ErrorCode init(const vec2<U16>& inputAreaDimensions);

    U8 update(const U64 deltaTimeUS);
    void terminate();

    void onChangeWindowSize(U16 w, U16 h);

    inline bool isInit() const { return _bIsInitialized; }

    inline void stop() { _bMustStop = true; }

    inline JoystickInterface* getJoystickInterface() { return _pJoystickInterface; }

    inline EffectManager& getEffectManager() { return *_pEffectMgr; }

    inline I32 joystickCount() const { return to_I32(_joysticks.size()); }
    inline OIS::JoyStick* getJoystick(Joystick index) const { return _joysticks[to_I32(index)]; }

    inline I32 kbMousePairCount() const { return to_I32(_kbMouseDevices.size()); }
    inline KBMousePair getKeyboardMousePair(U8 index) const { return _kbMouseDevices[index]; }

    inline bool isKeyDown(U8 deviceIndex, Input::KeyCode keyCode) const {
        if (!_kbMouseDevices[deviceIndex].first) {
            return false;
        }
        return _kbMouseDevices[deviceIndex].first->isKeyDown(keyCode);
    }

    inline bool isModifierDown(U8 deviceIndex, KeyModifier keyModifier) const {
        if (!_kbMouseDevices[deviceIndex].first) {
            return false;
        }
        return _kbMouseDevices[deviceIndex].first->isModifierDown(keyModifier);
    }

    inline const KeyEvent& getKey(KeyCode keyCode) const { return _keys[to_U32(keyCode)]; }

    static OIS::KeyCode keyCodeByName(const stringImpl& keyName);
    static MouseButton mouseButtonByName(const stringImpl& buttonName);
    static JoystickElement joystickElementByName(const stringImpl& elementName);

    InputState getKeyState(U8 deviceIndex, KeyCode keyCode) const;
    InputState getMouseButtonState(U8 deviceIndex, MouseButton button) const;
    InputState getJoystickeButtonState(Input::Joystick deviceIndex, JoystickButton button) const;

    Joystick joystick(I32 deviceID) const;
    I32 keyboard(I32 deviceID) const;
    I32 mouse(I32 deviceID) const;

protected:
    inline KeyEvent& getKeyRef(U32 index) { return _keys[index]; }
    inline DisplayWindow& parentWindow() { return _parent; }

protected:
    DisplayWindow& _parent;

    OIS::InputManager* _pInputInterface;
    EventHandler* _pEventHdlr;

    vectorImpl<KBMousePair> _kbMouseDevices;
    hashMapImpl<I32, I32> _keyboardIDToEntry;
    hashMapImpl<I32, I32> _mouseIdToEntry;

    /// multiple joystick support
    vectorImpl<OIS::JoyStick*> _joysticks;
    hashMapImpl<I32, Joystick> _joystickIdToEntry;

    JoystickInterface* _pJoystickInterface;
    EffectManager* _pEffectMgr;

    bool _bMustStop;
    bool _bIsInitialized;

    vectorImpl<KeyEvent> _keys;

};
namespace Attorney {
    class InputInterfaceEvent {
    private:
        static KeyEvent& getKeyRef(InputInterface& input, U32 index) {
            return input.getKeyRef(index);
        }

        static DisplayWindow& parentWindow(InputInterface& input) {
            return input.parentWindow();
        }

        friend class Divide::Input::EventHandler;
    };
};  // namespace Attorney


};  // namespace Input
};  // namespace Divide
#endif

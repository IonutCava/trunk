#include "stdafx.h"

#include "Headers/Scene.h"
#include "Headers/SceneInput.h"

#include "Core/Headers/PlatformContext.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

namespace {
    constexpr bool g_recordInput = true;
};

void PressReleaseActionCbks::from(const PressReleaseActions& actions, const InputActionList& actionList) {

    _entries.reserve(actions.entries().size());
    for (const PressReleaseActions::Entry& entry : actions.entries()) {
        Entry& newEntry = _entries.emplace_back();
        newEntry._actions[to_base(PressReleaseActions::Action::PRESS)].reserve(entry.pressIDs().size());
        newEntry._actions[to_base(PressReleaseActions::Action::RELEASE)].reserve(entry.releaseIDs().size());
        newEntry._modifiers = entry.modifiers();

        for (const U16 id : entry.pressIDs()) {
            newEntry._actions[to_base(PressReleaseActions::Action::PRESS)].push_back(actionList.getInputAction(id)._action);
        }
        for (const U16 id : entry.releaseIDs()) {
            newEntry._actions[to_base(PressReleaseActions::Action::RELEASE)].push_back(actionList.getInputAction(id)._action);
        }
    }
}

SceneInput::SceneInput(Scene& parentScene, PlatformContext& context) 
    : _parentScene(parentScene),
      _context(context)
{
}


void SceneInput::onSetActive() {
    
}

void SceneInput::onRemoveActive() {

}

void SceneInput::onPlayerAdd(U8 index) {
    hashAlg::insert(_keyLog, index, KeyLog());
    hashAlg::insert(_mouseBtnLog, index, MouseBtnLog());
}

void SceneInput::onPlayerRemove(U8 index) {
    _keyLog.find(index)->second.clear();
    _mouseBtnLog.find(index)->second.clear();
}

U8 SceneInput::getPlayerIndexForDevice(U8 deviceIndex) const {
    return deviceIndex;
}

bool SceneInput::handleCallbacks(const PressReleaseActionCbks& cbks,
                                 const InputParams& params,
                                 bool onPress)
{
    for (const PressReleaseActionCbks::Entry& entry : cbks._entries) {
        //Check modifiers
        for (const Input::KeyCode modifier : entry._modifiers) {
            if (Input::getKeyState(params._deviceIndex, modifier) != Input::InputState::PRESSED) {
                goto next;
            }
        }

        {
            const auto& actions = entry._actions[to_base(onPress ? PressReleaseActions::Action::PRESS 
                                                                 : PressReleaseActions::Action::RELEASE)];
            if (!actions.empty()) {
                for (const auto& action : actions) {
                    action(params);
                }

                return true;
            }
        }

    next:
        //Failed modifier test
        NOP();
    }

    return false;
}

bool SceneInput::onKeyDown(const Input::KeyEvent& arg) {
    if (g_recordInput) {
        _keyLog[arg._deviceIndex].emplace_back(std::make_pair(arg._key, Input::InputState::PRESSED));
    }

    PressReleaseActionCbks cbks;
    if (getKeyMapping(arg._key, cbks)) {
        return handleCallbacks(cbks,
                               InputParams(arg._deviceIndex, to_I32(arg._key), arg._modMask),
                               true);
    }

    return false;
}

bool SceneInput::onKeyUp(const Input::KeyEvent& arg) {
    if (g_recordInput) {
        _keyLog[arg._deviceIndex].emplace_back(std::make_pair(arg._key, Input::InputState::RELEASED));
    }

    PressReleaseActionCbks cbks;
    if (getKeyMapping(arg._key, cbks)) {
        return handleCallbacks(cbks,
                               InputParams(arg._deviceIndex, to_I32(arg._key), arg._modMask),
                               false);
    }

    return false;
}

bool SceneInput::joystickButtonPressed(const Input::JoystickEvent& arg) {

    Input::Joystick joy = static_cast<Input::Joystick>(arg._deviceIndex);
    PressReleaseActionCbks cbks;


    if (getJoystickMapping(joy, arg._element._type, arg._element._elementIndex, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_I32(arg._element._elementIndex)), true);
    }

    return false;
}

bool SceneInput::joystickButtonReleased(const Input::JoystickEvent& arg) {
    
    Input::Joystick joy = static_cast<Input::Joystick>(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, arg._element._type, arg._element._elementIndex, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_I32(arg._element._elementIndex)), false);
    }

    return false;
}

bool SceneInput::joystickAxisMoved(const Input::JoystickEvent& arg) {
    Input::Joystick joy = static_cast<Input::Joystick>(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, arg._element._type, arg._element._elementIndex, cbks)) {
        InputParams params(arg._deviceIndex,
                           arg._element._data._data, // move value
                           arg._element._elementIndex, // axis index
                           arg._element._data._gamePad ? 1 : 0, // is gamepad
                           arg._element._data._deadZone); // dead zone
        return handleCallbacks(cbks, params, true);
    }
    
    return false;
}

bool SceneInput::joystickPovMoved(const Input::JoystickEvent& arg) {

    Input::Joystick joy = static_cast<Input::Joystick>(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, arg._element._type, arg._element._elementIndex, cbks)) {
        InputParams params(arg._deviceIndex,
                           arg._element._data._data);
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::joystickBallMoved(const Input::JoystickEvent& arg) {
    Input::Joystick joy = static_cast<Input::Joystick>(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, arg._element._type, arg._element._elementIndex, cbks)) {
        InputParams params(arg._deviceIndex,
                           arg._element._data._smallData[0],
                           arg._element._data._smallData[1],
                           arg._element._data._gamePad ? 1 : 0);
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::joystickAddRemove(const Input::JoystickEvent& arg) {
    return false;
}

bool SceneInput::joystickRemap(const Input::JoystickEvent &arg) {
    return false;
}

bool SceneInput::mouseMoved(const Input::MouseMoveEvent& arg) {
    if (!arg.wheelEvent()) {
        PlayerIndex idx = getPlayerIndexForDevice(arg._deviceIndex);
        SceneStatePerPlayer& state = _parentScene.state().playerState(idx);
        if (state.cameraLockedToMouse()) {
            
            if (arg.wheelEvent()) {
                I32 wheel = arg.WheelV();
                state.zoom((wheel > 0) ? MoveDirection::POSITIVE
                                     : (wheel < 0) ? MoveDirection::NEGATIVE
                                                   : MoveDirection::NONE);
            } else {
                I32 xRel = arg.relativePos().x;
                I32 yRel = arg.relativePos().y;
                state.angleLR((xRel > 0) ? MoveDirection::POSITIVE
                                         : (xRel < 0) ? MoveDirection::NEGATIVE
                                                      : MoveDirection::NONE);

                state.angleUD((yRel > 0) ? MoveDirection::POSITIVE
                                                        : (yRel < 0) ? MoveDirection::NEGATIVE
                                                                     : MoveDirection::NONE);
            }
        }
    }

    return Attorney::SceneInput::mouseMoved(_parentScene, arg);
}

bool SceneInput::mouseButtonPressed(const Input::MouseButtonEvent& arg) {

    if (g_recordInput) {
        _mouseBtnLog[arg._deviceIndex].emplace_back(
            std::make_tuple(arg.button(), Input::InputState::PRESSED, vec2<I32>(arg.absPosition().x, arg.absPosition().y)));
    }

    PressReleaseActionCbks cbks = {};
    if (getMouseMapping(arg.button(), cbks)) {
        InputParams params(arg._deviceIndex, to_I32(arg.button()));
        params._var[2] = arg.absPosition().x;
        params._var[3] = arg.absPosition().y;
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    if (g_recordInput) {
        _mouseBtnLog[arg._deviceIndex].emplace_back(
                                std::make_tuple(arg.button(), Input::InputState::RELEASED, vec2<I32>(arg.absPosition().x, arg.absPosition().y)));
    }

    PressReleaseActionCbks cbks = {};
    if (getMouseMapping(arg.button(), cbks)) {
        InputParams params(arg._deviceIndex, to_I32(arg.button()));
        params._var[2] = arg.absPosition().x;
        params._var[3] = arg.absPosition().y;
        return handleCallbacks(cbks, params, false);
    }

    return false;
}

bool SceneInput::onUTF8(const Input::UTF8Event& arg) {
    ACKNOWLEDGE_UNUSED(arg);

    return false;
}

bool SceneInput::addKeyMapping(Input::KeyCode key, const PressReleaseActions::Entry& keyCbks) {
    return _keyMap[key].add(keyCbks);
}

bool SceneInput::removeKeyMapping(Input::KeyCode key) {
    KeyMap::iterator it = _keyMap.find(key);
    if (it != std::end(_keyMap)) {
        _keyMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getKeyMapping(Input::KeyCode key, PressReleaseActionCbks& keyCbksOut) {
    KeyMapCache::const_iterator itCache = _keyMapCache.find(key);
    if (itCache != std::cend(_keyMapCache)) {
        keyCbksOut = itCache->second;
        return true;
    }

    KeyMap::const_iterator it = _keyMap.find(key);
    if (it != std::cend(_keyMap)) {
        const PressReleaseActions& actions = it->second;
        keyCbksOut.from(actions, _actionList);
        hashAlg::insert(_keyMapCache, key, keyCbksOut);

        return true;
    }

    return false;
}

bool SceneInput::addMouseMapping(Input::MouseButton button, const PressReleaseActions::Entry& btnCbks) {
    return _mouseMap[button].add(btnCbks);
}

bool SceneInput::removeMouseMapping(Input::MouseButton button) {
    MouseMap::iterator it = _mouseMap.find(button);
    if (it != std::end(_mouseMap)) {
        _mouseMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getMouseMapping(Input::MouseButton button, PressReleaseActionCbks& btnCbksOut) {

    MouseMapCache::const_iterator itCache = _mouseMapCache.find(button);
    if (itCache != std::cend(_mouseMapCache)) {
        btnCbksOut = itCache->second;
        return true;
    }


    MouseMap::const_iterator it = _mouseMap.find(button);
    if (it != std::cend(_mouseMap)) {
        const PressReleaseActions& actions = it->second;
        btnCbksOut.from(actions, _actionList);
        hashAlg::insert(_mouseMapCache, button, btnCbksOut);
        return true;
    }

    return false;
}

bool SceneInput::addJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id, const PressReleaseActions::Entry& btnCbks) {
    const JoystickMapKey key = std::make_pair(to_base(elementType), id);
    return _joystickMap[to_base(device)][key].add(btnCbks);
}

bool SceneInput::removeJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id) {
    JoystickMapEntry& entry = _joystickMap[to_base(device)];

    JoystickMapEntry::iterator it = entry.find(std::make_pair(to_base(elementType), id));
    if (it != std::end(entry)) {
        entry.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id, PressReleaseActionCbks& btnCbksOut) {
    JoystickMapCacheEntry& entry = _joystickMapCache[to_base(device)];

    JoystickMapCacheEntry::const_iterator itCache = entry.find(std::make_pair(to_base(elementType), id));
    if (itCache != std::cend(entry)) {
        btnCbksOut = itCache->second;
        return true;
    }

    JoystickMapEntry& entry2 = _joystickMap[to_base(device)];
    JoystickMapEntry::const_iterator it = entry2.find(std::make_pair(to_base(elementType), id));
    if (it != std::cend(entry2)) {
        const PressReleaseActions& actions = it->second;
        btnCbksOut.from(actions, _actionList);
        JoystickMapKey key = std::make_pair(to_base(elementType), id);
        entry[key] = btnCbksOut;
        return true;
    }

    return false;
}

InputActionList& SceneInput::actionList() {
    return _actionList;
}

void SceneInput::flushCache() {
    _keyMapCache.clear();
    _mouseMapCache.clear();
    _joystickMapCache.clear();
}

};
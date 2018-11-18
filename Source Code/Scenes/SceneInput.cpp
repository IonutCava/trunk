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
    _onPressAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::PRESS))._action;
    _onReleaseAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::RELEASE))._action;
    _onLCtrlPressAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::LEFT_CTRL_PRESS))._action;
    _onLCtrlReleaseAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::LEFT_CTRL_RELEASE))._action;
    _onRCtrlPressAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::RIGHT_CTRL_PRESS))._action;
    _onRCtrlReleaseAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::RIGHT_CTRL_RELEASE))._action;
    _onLAltPressAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::LEFT_ALT_PRESS))._action;
    _onLAltReleaseAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::LEFT_ALT_RELEASE))._action;
    _onRAltPressAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::RIGHT_ALT_PRESS))._action;
    _onRAltReleaseAction = actionList.getInputAction(actions.actionID(PressReleaseActions::Action::RIGHT_ALT_RELEASE))._action;
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
    const DELEGATE_CBK<void, InputParams>& lAlt  = (onPress ? cbks._onLAltPressAction  : cbks._onLAltReleaseAction);
    const DELEGATE_CBK<void, InputParams>& rAlt  = (onPress ? cbks._onRAltPressAction  : cbks._onRAltReleaseAction);
    const DELEGATE_CBK<void, InputParams>& lCtrl = (onPress ? cbks._onLCtrlPressAction : cbks._onLCtrlReleaseAction);
    const DELEGATE_CBK<void, InputParams>& rCtrl = (onPress ? cbks._onRCtrlPressAction : cbks._onRCtrlReleaseAction);
    const DELEGATE_CBK<void, InputParams>& noMod = (onPress ? cbks._onPressAction      : cbks._onReleaseAction);

    if (Input::getKeyState(params._deviceIndex, Input::KeyCode::KC_LMENU) == Input::InputState::PRESSED && lAlt) {
        lAlt(params);
        return true;
    }

    if (Input::getKeyState(params._deviceIndex, Input::KeyCode::KC_RMENU) == Input::InputState::PRESSED && rAlt) {
        rAlt(params);
        return true;
    }

    if (Input::getKeyState(params._deviceIndex, Input::KeyCode::KC_LCONTROL) == Input::InputState::PRESSED && lCtrl) {
        lCtrl(params);
        return true;
    }

    if (Input::getKeyState(params._deviceIndex, Input::KeyCode::KC_RCONTROL) == Input::InputState::PRESSED && rCtrl) {
        rCtrl(params);
        return true;
    }

    if (noMod) {
        noMod(params);
        return true;
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
                state.zoom(wheel > 0 ? MoveDirection::POSITIVE
                                     : wheel < 0 ? MoveDirection::NEGATIVE
                                                 : MoveDirection::NONE);
            } else {
                I32 xRel = arg.relativePos().x;
                I32 yRel = arg.relativePos().y;
                state.angleLR(xRel > 1 ? MoveDirection::POSITIVE
                                       : xRel < 1 ? MoveDirection::NEGATIVE
                                                  : MoveDirection::NONE);

                state.angleUD(yRel > 1 ? MoveDirection::POSITIVE
                                       : yRel < 1 ? MoveDirection::NEGATIVE
                                                  : MoveDirection::NONE);
            }
        }
    }

    return Attorney::SceneInput::mouseMoved(_parentScene, arg);
}

bool SceneInput::mouseButtonPressed(const Input::MouseButtonEvent& arg) {

    if (g_recordInput) {
        _mouseBtnLog[arg._deviceIndex].emplace_back(
            vectorAlg::make_tuple(arg.button, Input::InputState::PRESSED, vec2<I32>(arg.relPosition.x, arg.relPosition.y)));
    }

    PressReleaseActionCbks cbks;
    if (getMouseMapping(arg.button, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_I32(arg.button)), true);
    }

    return false;
}

bool SceneInput::mouseButtonReleased(const Input::MouseButtonEvent& arg) {
    if (g_recordInput) {
        _mouseBtnLog[arg._deviceIndex].emplace_back(
                                vectorAlg::make_tuple(arg.button, Input::InputState::RELEASED, vec2<I32>(arg.relPosition.x, arg.relPosition.y)));
    }

    PressReleaseActionCbks cbks;
    if (getMouseMapping(arg.button, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_I32(arg.button)), false);
    }

    return false;
}

bool SceneInput::onUTF8(const Input::UTF8Event& arg) {
    ACKNOWLEDGE_UNUSED(arg);

    return false;
}

bool SceneInput::addKeyMapping(Input::KeyCode key, PressReleaseActions keyCbks) {
    auto result = hashAlg::insert(_keyMap, key, keyCbks);
    if (!result.second) {
        return result.first->second.merge(keyCbks);
    }

    return result.second;
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

bool SceneInput::addMouseMapping(Input::MouseButton button, PressReleaseActions btnCbks) {
    auto result = hashAlg::insert(_mouseMap, button, btnCbks);

    return result.second;
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

bool SceneInput::addJoystickMapping(Input::Joystick device, Input::JoystickElementType elementType, U32 id, PressReleaseActions btnCbks) {
    if (elementType == Input::JoystickElementType::POV_MOVE ||
        elementType == Input::JoystickElementType::AXIS_MOVE ||
        elementType == Input::JoystickElementType::BALL_MOVE)
    {
        // non-buttons have no pressed/release states so map on release to on press's action as well
        if (btnCbks.actionID(PressReleaseActions::Action::PRESS)){ 
            btnCbks.actionID(PressReleaseActions::Action::RELEASE, btnCbks.actionID(PressReleaseActions::Action::PRESS));
        } else {
            btnCbks.actionID(PressReleaseActions::Action::PRESS, btnCbks.actionID(PressReleaseActions::Action::RELEASE));
        }

        if (btnCbks.actionID(PressReleaseActions::Action::LEFT_CTRL_PRESS)) {
            btnCbks.actionID(PressReleaseActions::Action::LEFT_CTRL_RELEASE, btnCbks.actionID(PressReleaseActions::Action::LEFT_CTRL_PRESS));
        } else {
            btnCbks.actionID(PressReleaseActions::Action::LEFT_CTRL_PRESS, btnCbks.actionID(PressReleaseActions::Action::LEFT_CTRL_RELEASE));
        }

        if (btnCbks.actionID(PressReleaseActions::Action::RIGHT_CTRL_PRESS)) {
            btnCbks.actionID(PressReleaseActions::Action::RIGHT_CTRL_RELEASE, btnCbks.actionID(PressReleaseActions::Action::RIGHT_CTRL_PRESS));
        } else {
            btnCbks.actionID(PressReleaseActions::Action::RIGHT_CTRL_PRESS, btnCbks.actionID(PressReleaseActions::Action::RIGHT_CTRL_RELEASE));
        }

        if (btnCbks.actionID(PressReleaseActions::Action::LEFT_ALT_PRESS)) {
            btnCbks.actionID(PressReleaseActions::Action::LEFT_ALT_RELEASE, btnCbks.actionID(PressReleaseActions::Action::LEFT_ALT_PRESS));
        } else {
            btnCbks.actionID(PressReleaseActions::Action::LEFT_ALT_PRESS, btnCbks.actionID(PressReleaseActions::Action::LEFT_ALT_RELEASE));
        }

        if (btnCbks.actionID(PressReleaseActions::Action::RIGHT_ALT_PRESS)) {
            btnCbks.actionID(PressReleaseActions::Action::RIGHT_ALT_RELEASE, btnCbks.actionID(PressReleaseActions::Action::RIGHT_ALT_PRESS));
        } else {
            btnCbks.actionID(PressReleaseActions::Action::RIGHT_ALT_PRESS, btnCbks.actionID(PressReleaseActions::Action::RIGHT_ALT_RELEASE));
        }
    }
    JoystickMapKey key = std::make_pair(to_base(elementType), id);
    bool existing = _joystickMap[to_base(device)].find(key) != std::cend(_joystickMap[to_base(device)]);
    _joystickMap[to_base(device)][key] = btnCbks;

    return existing;
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
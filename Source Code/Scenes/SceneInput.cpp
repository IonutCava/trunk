#include "Headers/Scene.h"
#include "Headers/SceneInput.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

namespace {
    const bool g_recordInput = true;
};

void PressReleaseActionCbks::from(const PressReleaseActions& actions, const InputActionList& actionList) {
    _onPressAction = actionList.getInputAction(actions._onPressAction)._action;
    _onReleaseAction = actionList.getInputAction(actions._onReleaseAction)._action;
    _onLCtrlPressAction = actionList.getInputAction(actions._onLCtrlPressAction)._action;
    _onLCtrlReleaseAction = actionList.getInputAction(actions._onLCtrlReleaseAction)._action;
    _onRCtrlPressAction = actionList.getInputAction(actions._onRCtrlPressAction)._action;
    _onRCtrlReleaseAction = actionList.getInputAction(actions._onRCtrlReleaseAction)._action;
    _onLAltPressAction = actionList.getInputAction(actions._onLAltPressAction)._action;
    _onLAltReleaseAction = actionList.getInputAction(actions._onLAltReleaseAction)._action;
    _onRAltPressAction = actionList.getInputAction(actions._onRAltPressAction)._action;
    _onRAltReleaseAction = actionList.getInputAction(actions._onRAltReleaseAction)._action;
}

SceneInput::SceneInput(Scene& parentScene, Input::InputInterface& context) 
    : _parentScene(parentScene),
      _context(context)
{
}

bool SceneInput::isDeviceInUse(I32 deviceID) {
    return std::find_if(std::cbegin(_usedInputDevices),
                        std::cend(_usedInputDevices),
                        [deviceID](I32 device) {
                            return device == deviceID;
                        }) != std::cend(_usedInputDevices);
}

void SceneInput::onSetActive() {
    _playerControlDevices.clear();
    _usedInputDevices.clear();
}

void SceneInput::onRemoveActive() {

}

void SceneInput::onPlayerAdd(U8 index) {
    _keyLog.insert(std::make_pair(index, KeyLog()));
    _mouseBtnLog.insert(std::make_pair(index, MouseBtnLog()));

    std::pair<I32, I32>& devices = _playerControlDevices[index];
    
    // Keyboard + Mouse pairs have priority
    I32 numKBMousePairs = _context.kbMousePairCount();
    for (I32 i = 0; i < numKBMousePairs; ++i) {
        Input::InputInterface::KBMousePair pair = _context.getKeyboardMousePair(to_ubyte(i));

        std::pair<I32, I32> deviceIndices(pair.first ? pair.first->getID() : -1,
                                          pair.second ? pair.second->getID() : -1);

        if (!isDeviceInUse(deviceIndices.first) && !isDeviceInUse(deviceIndices.second)) {
            devices = deviceIndices;
            _usedInputDevices.push_back(devices.first);
            _usedInputDevices.push_back(devices.second);
            return;
        }
    }

    // Fallback to gamepads
    I32 numJoysticks = _context.joystickCount();
    for (I32 i = 0; i < numJoysticks; ++i) {
        OIS::JoyStick* joy = _context.getJoystick(static_cast<Input::Joystick>(i));
        I32 deviceIndex = joy ? joy->getID() : -1;
        if (!isDeviceInUse(deviceIndex)) {
            devices.first = deviceIndex;
            devices.second = -1;
            _usedInputDevices.push_back(deviceIndex);
            return;
        }
    }
}

void SceneInput::onPlayerRemove(U8 index) {
    _keyLog.find(index)->second.clear();
    _mouseBtnLog.find(index)->second.clear();

    std::pair<I32, I32>& devices = _playerControlDevices[index];
    _usedInputDevices.erase(std::remove_if(std::begin(_usedInputDevices),
                                           std::end(_usedInputDevices),
                                           [devices](I32 device) {
                                                return device == devices.first ||
                                                       device == devices.second;
                                           }),
                            std::end(_usedInputDevices));

    devices.first = devices.second = -1;
}

U8 SceneInput::getPlayerIndexForDevice(U8 deviceIndex) const {
    for (hashMapImpl<U8 /*player index*/, std::pair<I32, I32>>::value_type it : _playerControlDevices) {
        if (it.second.first == deviceIndex || it.second.second == deviceIndex) {
            return it.first;
        }
    }

    DIVIDE_UNEXPECTED_CALL("No player for current device");
    return 0;
}

Input::InputState SceneInput::getKeyState(U8 deviceIndex, Input::KeyCode key) const {
    return _context.getKeyState(deviceIndex, key);
}

Input::InputState SceneInput::getMouseButtonState(U8 deviceIndex, Input::MouseButton button) const {
   return _context.getMouseButtonState(deviceIndex, button);
}

Input::InputState SceneInput::getJoystickButtonState(Input::Joystick deviceIndex,
                                                     Input::JoystickButton button) const {
    return _context.getJoystickeButtonState(deviceIndex, button);
}

bool SceneInput::handleCallbacks(const PressReleaseActionCbks& cbks,
                                 const InputParams& params,
                                 bool onPress)
{
    const DELEGATE_CBK_PARAM<InputParams>& lAlt  = (onPress ? cbks._onLAltPressAction  : cbks._onLAltReleaseAction);
    const DELEGATE_CBK_PARAM<InputParams>& rAlt  = (onPress ? cbks._onRAltPressAction  : cbks._onRAltReleaseAction);
    const DELEGATE_CBK_PARAM<InputParams>& lCtrl = (onPress ? cbks._onLCtrlPressAction : cbks._onLCtrlReleaseAction);
    const DELEGATE_CBK_PARAM<InputParams>& rCtrl = (onPress ? cbks._onRCtrlPressAction : cbks._onRCtrlReleaseAction);
    const DELEGATE_CBK_PARAM<InputParams>& noMod = (onPress ? cbks._onPressAction      : cbks._onReleaseAction);

    if (getKeyState(params._deviceIndex, Input::KeyCode::KC_LMENU) == Input::InputState::PRESSED && lAlt) {
        lAlt(params);
        return true;
    }

    if (getKeyState(params._deviceIndex, Input::KeyCode::KC_RMENU) == Input::InputState::PRESSED && rAlt) {
        rAlt(params);
        return true;
    }

    if (getKeyState(params._deviceIndex, Input::KeyCode::KC_LCONTROL) == Input::InputState::PRESSED && lCtrl) {
        lCtrl(params);
        return true;
    }

    if (getKeyState(params._deviceIndex, Input::KeyCode::KC_RCONTROL) == Input::InputState::PRESSED && rCtrl) {
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
        vectorAlg::emplace_back(
            _keyLog[arg._deviceIndex], std::make_pair(arg._key, Input::InputState::PRESSED));
    }

    PressReleaseActionCbks cbks;
    if (getKeyMapping(arg._key, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, arg._deviceIndex, to_int(arg._key)), true);
    }

    return false;
}

bool SceneInput::onKeyUp(const Input::KeyEvent& arg) {
    if (g_recordInput) {
        vectorAlg::emplace_back(
            _keyLog[arg._deviceIndex], std::make_pair(arg._key, Input::InputState::RELEASED));
    }

    PressReleaseActionCbks cbks;
    if (getKeyMapping(arg._key, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_int(arg._key)), false);
    }

    return false;
}

bool SceneInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       Input::JoystickButton button) {

    Input::Joystick joy = _context.joystick(arg._deviceIndex);
    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::BUTTON_PRESS, button), cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_int(button)), true);
    }

    return false;
}

bool SceneInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        Input::JoystickButton button) {
    
    Input::Joystick joy = _context.joystick(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::BUTTON_PRESS, button), cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_int(button)), false);
    }

    return false;
}

bool SceneInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    Input::Joystick joy = _context.joystick(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::AXIS_MOVE, axis), cbks)) {
        InputParams params(arg._deviceIndex,
                           arg._event.state.mAxes[axis].abs,
                           arg._event.state.mAxes[axis].rel,
                           to_int(axis),
                           to_int(joy));
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {

    Input::Joystick joy = _context.joystick(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::POV_MOVE, pov), cbks)) {
        InputParams params(arg._deviceIndex, arg._event.state.mPOV[pov].direction);
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::joystickSliderMoved(const Input::JoystickEvent& arg, I8 index) {
    Input::Joystick joy = _context.joystick(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::SLIDER_MOVE, index), cbks)) {
        InputParams params(arg._deviceIndex,
                           arg._event.state.mSliders[index].abX,
                           arg._event.state.mSliders[index].abY);
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index) {
    Input::Joystick joy = _context.joystick(arg._deviceIndex);

    PressReleaseActionCbks cbks;
    if (getJoystickMapping(joy, Input::JoystickElement(Input::JoystickElementType::VECTOR_MOVE, index), cbks)) {
        InputParams params(arg._deviceIndex,
                           to_int(arg._event.state.mVectors[index].x),
                           to_int(arg._event.state.mVectors[index].y),
                           to_int(arg._event.state.mVectors[index].z));
        return handleCallbacks(cbks, params, true);
    }

    return false;
}

bool SceneInput::mouseMoved(const Input::MouseEvent& arg) {
    SceneStatePerPlayer& state = _parentScene.state().playerState(arg._deviceIndex);
    state.aimPos(vec2<I32>(arg._event.state.X.abs, arg._event.state.Y.abs));

    if (state.cameraLockedToMouse()) {
        state.angleLR(state.aimDelta().x < 0
                        ? MoveDirection::POSITIVE
                        : MoveDirection::NEGATIVE);
        state.angleUD(state.aimDelta().x < 0
                        ? MoveDirection::POSITIVE
                        : MoveDirection::NEGATIVE);
    }

    return Attorney::SceneInput::mouseMoved(_parentScene, arg);
}

bool SceneInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton id) {

    if (g_recordInput) {
        vectorAlg::emplace_back(_mouseBtnLog[arg._deviceIndex],
            vectorAlg::make_tuple(id, Input::InputState::PRESSED, vec2<I32>(arg._event.state.X.abs, arg._event.state.Y.abs)));
    }

    PressReleaseActionCbks cbks;
    if (getMouseMapping(id, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_int(id)), true);
    }

    return false;
}

bool SceneInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton id) {
    if (g_recordInput) {
        vectorAlg::emplace_back(_mouseBtnLog[arg._deviceIndex],
                                vectorAlg::make_tuple(id, Input::InputState::RELEASED, vec2<I32>(arg._event.state.X.abs, arg._event.state.Y.abs)));
    }

    PressReleaseActionCbks cbks;
    if (getMouseMapping(id, cbks)) {
        return handleCallbacks(cbks, InputParams(arg._deviceIndex, to_int(id)), false);
    }

    return false;
}

bool SceneInput::addKeyMapping(Input::KeyCode key, PressReleaseActions keyCbks) {
    std::pair<KeyMap::iterator, bool> result = hashAlg::emplace(_keyMap, key, keyCbks);

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
        hashAlg::emplace(_keyMapCache, key, keyCbksOut);

        return true;
    }

    return false;
}

bool SceneInput::addMouseMapping(Input::MouseButton button, PressReleaseActions btnCbks) {
    std::pair<MouseMap::iterator, bool> result =
        hashAlg::emplace(_mouseMap, button, btnCbks);

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
        hashAlg::emplace(_mouseMapCache, button, btnCbksOut);
        return true;
    }

    return false;
}

bool SceneInput::addJoystickMapping(Input::Joystick device, Input::JoystickElement element, PressReleaseActions btnCbks) {
    if (element._type == Input::JoystickElementType::POV_MOVE ||
        element._type == Input::JoystickElementType::AXIS_MOVE ||
        element._type == Input::JoystickElementType::SLIDER_MOVE ||
        element._type == Input::JoystickElementType::VECTOR_MOVE)
    {
        // non-buttons have no pressed/release states so map on release to on press's action as well
        if (btnCbks._onPressAction){ 
            btnCbks._onReleaseAction = btnCbks._onPressAction;
        } else {
            btnCbks._onPressAction = btnCbks._onReleaseAction;
        }

        if (btnCbks._onLCtrlPressAction) {
            btnCbks._onLCtrlReleaseAction = btnCbks._onLCtrlPressAction;
        } else {
            btnCbks._onLCtrlPressAction = btnCbks._onLCtrlReleaseAction;
        }

        if (btnCbks._onRCtrlPressAction) {
            btnCbks._onRCtrlReleaseAction = btnCbks._onRCtrlPressAction;
        } else {
            btnCbks._onRCtrlPressAction = btnCbks._onRCtrlReleaseAction;
        }

        if (btnCbks._onLAltPressAction) {
            btnCbks._onLAltReleaseAction = btnCbks._onLAltPressAction;
        } else {
            btnCbks._onLAltPressAction = btnCbks._onLAltReleaseAction;
        }

        if (btnCbks._onRAltPressAction) {
            btnCbks._onRAltReleaseAction = btnCbks._onRAltPressAction;
        } else {
            btnCbks._onRAltPressAction = btnCbks._onRAltReleaseAction;
        }
    }

    return hashAlg::emplace(_joystickMap[device], element, btnCbks).second;
}

bool SceneInput::removeJoystickMapping(Input::Joystick device, Input::JoystickElement element) {
    JoystickMapEntry& entry = _joystickMap[device];

    JoystickMapEntry::iterator it = entry.find(element);
    if (it != std::end(entry)) {
        entry.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getJoystickMapping(Input::Joystick device, Input::JoystickElement element, PressReleaseActionCbks& btnCbksOut) {
    JoystickMapCacheEntry& entry = _joystickMapCache[device];

    JoystickMapCacheEntry::const_iterator itCache = entry.find(element);
    if (itCache != std::cend(entry)) {
        btnCbksOut = itCache->second;
        return true;
    }

    JoystickMapEntry& entry2 = _joystickMap[device];
    JoystickMapEntry::const_iterator it = entry2.find(element);
    if (it != std::cend(entry2)) {
        const PressReleaseActions& actions = it->second;
        btnCbksOut.from(actions, _actionList);
        hashAlg::emplace(entry, element, btnCbksOut);
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
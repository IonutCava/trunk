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
    _onPressAction = actionList.getInputAction(actions._onPressAction);
    _onReleaseAction = actionList.getInputAction(actions._onReleaseAction);
    _onLCtrlPressAction = actionList.getInputAction(actions._onLCtrlPressAction);
    _onLCtrlReleaseAction = actionList.getInputAction(actions._onLCtrlReleaseAction);
    _onRCtrlPressAction = actionList.getInputAction(actions._onRCtrlPressAction);
    _onRCtrlReleaseAction = actionList.getInputAction(actions._onRCtrlReleaseAction);
    _onLAltPressAction = actionList.getInputAction(actions._onLAltPressAction);
    _onLAltReleaseAction = actionList.getInputAction(actions._onLAltReleaseAction);
    _onRAltPressAction = actionList.getInputAction(actions._onRAltPressAction);
    _onRAltReleaseAction = actionList.getInputAction(actions._onRAltReleaseAction);
}

SceneInput::SceneInput(Scene& parentScene) 
    : _parentScene(parentScene)
{
}

SceneInput::InputState SceneInput::getKeyState(Input::KeyCode key) const {
    KeyState::const_iterator it = _keyStateMap.find(key);
    if (it != std::end(_keyStateMap)) {
        return it->second;
    }

    return InputState::COUNT;
}

SceneInput::InputState SceneInput::getMouseButtonState(Input::MouseButton button) const {
    MouseBtnState::const_iterator it = _mouseStateMap.find(button);
    if (it != std::end(_mouseStateMap)) {
        return it->second;
    }

    return InputState::COUNT;
}

SceneInput::InputState SceneInput::getJoystickButtonState(Input::JoystickButton button) const {
    JoystickBtnState::const_iterator it = _joystickStateMap.find(button);
    if (it != std::end(_joystickStateMap)) {
        return it->second;
    }

    return InputState::COUNT;
}

bool SceneInput::onKeyDown(const Input::KeyEvent& arg) {
    if (g_recordInput) {
        vectorAlg::emplace_back(
            _keyLog, std::make_pair(arg._key, InputState::PRESSED));
    }

    _keyStateMap[arg._key] = InputState::PRESSED;
    PressReleaseActionCbks cbks;
    if (!getKeyMapping(arg._key, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltPressAction) {
            cbks._onLAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltPressAction) {
            cbks._onRAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlPressAction) {
            cbks._onLCtrlPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlPressAction) {
            cbks._onRCtrlPressAction();
            return true;
        }

        if (cbks._onPressAction) {
            cbks._onPressAction();
            return true;
        }
    }

    return false;
}

bool SceneInput::onKeyUp(const Input::KeyEvent& arg) {
    if (g_recordInput) {
        vectorAlg::emplace_back(
            _keyLog, std::make_pair(arg._key, InputState::RELEASED));
    }

    _keyStateMap[arg._key] = InputState::RELEASED;
    PressReleaseActionCbks cbks;
    if (!getKeyMapping(arg._key, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltReleaseAction) {
            cbks._onLAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltReleaseAction) {
            cbks._onRAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlReleaseAction) {
            cbks._onLCtrlReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlReleaseAction) {
            cbks._onRCtrlReleaseAction();
            return true;
        }

        if (cbks._onReleaseAction) {
            cbks._onReleaseAction();
            return true;
        }
    }

    return false;
}

bool SceneInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       Input::JoystickButton button) {
    _joystickStateMap[button] = InputState::PRESSED;
    PressReleaseActionCbks cbks;
    if (!getJoystickMapping(button, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltPressAction) {
            cbks._onLAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltPressAction) {
            cbks._onRAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlPressAction) {
            cbks._onLCtrlPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlPressAction) {
            cbks._onRCtrlPressAction();
            return true;
        }

        if (cbks._onPressAction) {
            cbks._onPressAction();
            return true;
        }
    }

    return false;
}

bool SceneInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        Input::JoystickButton button) {
    _joystickStateMap[button] = InputState::RELEASED;
    PressReleaseActionCbks cbks;
    if (!getJoystickMapping(button, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltReleaseAction) {
            cbks._onLAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltReleaseAction) {
            cbks._onRAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlReleaseAction) {
            cbks._onLCtrlReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlReleaseAction) {
            cbks._onRCtrlReleaseAction();
            return true;
        }

        if (cbks._onReleaseAction) {
            cbks._onReleaseAction();
            return true;
        }
    }

    return false;
}

bool SceneInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    STUBBED("ToDo: Store input from multiple joysticks in scene state! - Ionut");

    SceneState& state = _parentScene.state();
    Input::Joystick joystick = static_cast<Input::Joystick>(arg.device->getID());
    if (joystick != Input::Joystick::JOYSTICK_1) {
        return false;
    }

    Input::JoystickInterface* joyInterface = nullptr;
    joyInterface = Input::InputInterface::instance().getJoystickInterface();
    const Input::JoystickData& joyData = joyInterface->getJoystickData(joystick);
    I32 deadZone = joyData._deadZone;
    I32 axisABS = std::min(arg.state.mAxes[axis].abs, joyData._max);

    switch (axis) {
        case 0: {
            if (axisABS > deadZone) {
                state.angleUD(SceneState::MoveDirection::POSITIVE);
            } else if (axisABS < -deadZone) {
                state.angleUD(SceneState::MoveDirection::NEGATIVE);
            } else {
                state.angleUD(SceneState::MoveDirection::NONE);
            }
        } break;
        case 1: {
            if (axisABS > deadZone) {
                state.angleLR(SceneState::MoveDirection::POSITIVE);
            } else if (axisABS < -deadZone) {
                state.angleLR(SceneState::MoveDirection::NEGATIVE);
            } else {
                state.angleLR(SceneState::MoveDirection::NONE);
            }
        } break;

        case 2: {
            if (axisABS < -deadZone) {
                state.moveFB(SceneState::MoveDirection::POSITIVE);
            } else if (axisABS > deadZone) {
                state.moveFB(SceneState::MoveDirection::NEGATIVE);
            } else {
                state.moveFB(SceneState::MoveDirection::NONE);
            }
        } break;
        case 3: {
            if (axisABS < -deadZone) {
                state.moveLR(SceneState::MoveDirection::NEGATIVE);
            } else if (axisABS > deadZone) {
                state.moveLR(SceneState::MoveDirection::POSITIVE);
            } else {
                state.moveLR(SceneState::MoveDirection::NONE);
            }
        } break;
    }
    return true;
}

bool SceneInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    SceneState& state = _parentScene.state();
    if (arg.state.mPOV[pov].direction & OIS::Pov::North) {  // Going up
        state.moveFB(SceneState::MoveDirection::POSITIVE);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::South) {  // Going down
        state.moveFB(SceneState::MoveDirection::NEGATIVE);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::East) {  // Going right
        state.moveLR(SceneState::MoveDirection::POSITIVE);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::West) {  // Going left
        state.moveLR(SceneState::MoveDirection::NEGATIVE);
    }
    if (arg.state.mPOV[pov].direction == OIS::Pov::Centered) {  // stopped/centered out
        state.moveLR(SceneState::MoveDirection::NONE);
        state.moveFB(SceneState::MoveDirection::NONE);
    }
    return true;
}

bool SceneInput::joystickSliderMoved(const Input::JoystickEvent&, I8 index) {
    return true;
}

bool SceneInput::joystickVector3DMoved(const Input::JoystickEvent& arg,
                                       I8 index) {
    return true;
}

bool SceneInput::mouseMoved(const Input::MouseEvent& arg) {
    SceneState& state = _parentScene.state();
    state.mouseXDelta(_mousePos.x - arg.state.X.abs);
    state.mouseYDelta(_mousePos.y - arg.state.Y.abs);
    _mousePos.set(arg.state.X.abs, arg.state.Y.abs);

    if (state.cameraLockedToMouse()) {
        state.angleLR(state.mouseXDelta() < 0 
                        ? SceneState::MoveDirection::POSITIVE
                        : SceneState::MoveDirection::NEGATIVE);
        state.angleUD(state.mouseYDelta() < 0 
                        ? SceneState::MoveDirection::POSITIVE
                        : SceneState::MoveDirection::NEGATIVE);
    }

    return true;
}

bool SceneInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton id) {

    if (g_recordInput) {
        vectorAlg::emplace_back(_mouseBtnLog,
            vectorAlg::make_tuple(id, InputState::PRESSED, getMousePosition()));
    }

    _mouseStateMap[id] = InputState::PRESSED;
    PressReleaseActionCbks cbks;
    if (!getMouseMapping(id, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltPressAction) {
            cbks._onLAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltPressAction) {
            cbks._onRAltPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlPressAction) {
            cbks._onLCtrlPressAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlPressAction) {
            cbks._onRCtrlPressAction();
            return true;
        }

        if (cbks._onPressAction) {
            cbks._onPressAction();
            return true;
        }
    }

    return false;
}

bool SceneInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton id) {
    if (g_recordInput) {
        vectorAlg::emplace_back(_mouseBtnLog,
                                vectorAlg::make_tuple(id, InputState::RELEASED,   getMousePosition()));
    }

    _mouseStateMap[id] = InputState::RELEASED;
    PressReleaseActionCbks cbks;
    if (!getMouseMapping(id, cbks)) {
        if (getKeyState(Input::KeyCode::KC_LMENU) == InputState::PRESSED && cbks._onLAltReleaseAction) {
            cbks._onLAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RMENU) == InputState::PRESSED && cbks._onRAltReleaseAction) {
            cbks._onRAltReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_LCONTROL) == InputState::PRESSED && cbks._onLCtrlReleaseAction) {
            cbks._onLCtrlReleaseAction();
            return true;
        }

        if (getKeyState(Input::KeyCode::KC_RCONTROL) == InputState::PRESSED && cbks._onRCtrlReleaseAction) {
            cbks._onRCtrlReleaseAction();
            return true;
        }

        if (cbks._onReleaseAction) {
            cbks._onReleaseAction();
            return true;
        }
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

bool SceneInput::addJoystickMapping(Input::JoystickButton button, PressReleaseActions btnCbks) {
    std::pair<JoystickMap::iterator, bool> result =
        hashAlg::emplace(_joystickMap, button, btnCbks);

    return result.second;
}

bool SceneInput::removeJoystickMapping(Input::JoystickButton button) {
    JoystickMap::iterator it = _joystickMap.find(button);
    if (it != std::end(_joystickMap)) {
        _joystickMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getJoystickMapping(Input::JoystickButton button, PressReleaseActionCbks& btnCbksOut) {

    JoystickMapCache::const_iterator itCache = _joystickMapCache.find(button);
    if (itCache != std::cend(_joystickMapCache)) {
        btnCbksOut = itCache->second;
        return true;
    }

    JoystickMap::const_iterator it = _joystickMap.find(button);
    if (it != std::cend(_joystickMap)) {
        const PressReleaseActions& actions = it->second;
        btnCbksOut.from(actions, _actionList);
        hashAlg::emplace(_joystickMapCache, button, btnCbksOut);
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
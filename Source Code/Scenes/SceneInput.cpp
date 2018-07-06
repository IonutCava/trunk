#include "Headers/Scene.h"
#include "Headers/SceneInput.h"

#include "Core/Headers/ParamHandler.h"
#include "Rendering/PostFX/Headers/PostFX.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

static const I16 axisDeadZone = 256;

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
    _keyStateMap[arg._key] = InputState::PRESSED;
    PressReleaseActions cbks;
    if (getKeyMapping(arg._key, cbks) && cbks.first){
        cbks.first();
    }

    return true;
}

bool SceneInput::onKeyUp(const Input::KeyEvent& arg) {
    _keyStateMap[arg._key] = InputState::RELEASED;
    PressReleaseActions cbks;
    if (getKeyMapping(arg._key, cbks) && cbks.second){
        cbks.second();
    }
    return true;
}

bool SceneInput::joystickButtonPressed(const Input::JoystickEvent& arg,
                                       Input::JoystickButton button) {
    _joystickStateMap[button] = InputState::PRESSED;
    PressReleaseActions cbks;
    if (getJoystickMapping(button, cbks) && cbks.first){
        cbks.first();
    }
    return true;
}

bool SceneInput::joystickButtonReleased(const Input::JoystickEvent& arg,
                                        Input::JoystickButton button) {
    _joystickStateMap[button] = InputState::RELEASED;
    PressReleaseActions cbks;
    if (getJoystickMapping(button, cbks) && cbks.second){
        cbks.second();
    }
    return true;
}

bool SceneInput::joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis) {
    STUBBED("ToDo: Store input from multiple joysticks in scene state! - Ionut");

    SceneState& state = _parentScene.state();
    if (arg.device->getID() != to_int(Input::Joystick::JOYSTICK_1)) {
        return false;
    }
    I32 axisABS = arg.state.mAxes[axis].abs;

    switch (axis) {
        case 0: {
            if (axisABS > axisDeadZone) {
                state.angleUD(1);
            } else if (axisABS < -axisDeadZone) {
                state.angleUD(-1);
            } else {
                state.angleUD(0);
            }
        } break;
        case 1: {
            if (axisABS > axisDeadZone) {
                state.angleLR(1);
            } else if (axisABS < -axisDeadZone) {
                state.angleLR(-1);
            } else {
                state.angleLR(0);
            }
        } break;

        case 2: {
            if (axisABS < -axisDeadZone) {
                state.moveFB(1);
            } else if (axisABS > axisDeadZone) {
                state.moveFB(-1);
            } else {
                state.moveFB(0);
            }
        } break;
        case 3: {
            if (axisABS < -axisDeadZone) {
                state.moveLR(-1);
            } else if (axisABS > axisDeadZone) {
                state.moveLR(1);
            } else {
                state.moveLR(0);
            }
        } break;
    }
    return true;
}

bool SceneInput::joystickPovMoved(const Input::JoystickEvent& arg, I8 pov) {
    SceneState& state = _parentScene.state();
    if (arg.state.mPOV[pov].direction & OIS::Pov::North) {  // Going up
        state.moveFB(1);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::South) {  // Going down
        state.moveFB(-1);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::East) {  // Going right
        state.moveLR(1);
    }
    if (arg.state.mPOV[pov].direction & OIS::Pov::West) {  // Going left
        state.moveLR(-1);
    }
    if (arg.state.mPOV[pov].direction == OIS::Pov::Centered) {  // stopped/centered out
        state.moveLR(0);
        state.moveFB(0);
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

    if (getMouseButtonState(Input::MouseButton::MB_Right) ==
        InputState::PRESSED) {
        state.angleLR(-state.mouseXDelta());
        state.angleUD(-state.mouseYDelta());
    }

    return true;
}

bool SceneInput::mouseButtonPressed(const Input::MouseEvent& arg,
                                    Input::MouseButton id) {
    _mouseStateMap[id] = InputState::PRESSED;
    PressReleaseActions cbks;
    if (getMouseMapping(id, cbks) && cbks.first){
        cbks.first();
    }
    return true;
}

bool SceneInput::mouseButtonReleased(const Input::MouseEvent& arg,
                                     Input::MouseButton id) {
    _mouseStateMap[id] = InputState::RELEASED;
    PressReleaseActions cbks;
    if (getMouseMapping(id, cbks) && cbks.second){
        cbks.second();
    }
    return true;
}

bool SceneInput::addKeyMapping(Input::KeyCode key,
                               const PressReleaseActions& keyCbks) {
    hashAlg::pair<KeyMap::iterator, bool> result =
        hashAlg::emplace(_keyMap, key, keyCbks);

    return result.second;
}

bool SceneInput::removeKeyMapping(Input::KeyCode key) {
    KeyMap::const_iterator it = _keyMap.find(key);
    if (it != std::end(_keyMap)) {
        _keyMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getKeyMapping(Input::KeyCode key,
                               PressReleaseActions& keyCbksOut) const {
    KeyMap::const_iterator it = _keyMap.find(key);
    if (it != std::end(_keyMap)) {
        keyCbksOut = it->second;
        return true;
    }

    return false;
}

bool SceneInput::addMouseMapping(Input::MouseButton button,
                                 const PressReleaseActions& btnCbks) {
    hashAlg::pair<MouseMap::iterator, bool> result =
        hashAlg::emplace(_mouseMap, button, btnCbks);

    return result.second;
}

bool SceneInput::removeMouseMapping(Input::MouseButton button) {
    MouseMap::const_iterator it = _mouseMap.find(button);
    if (it != std::end(_mouseMap)) {
        _mouseMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getMouseMapping(Input::MouseButton button,
                                 PressReleaseActions& btnCbksOut) const {
    MouseMap::const_iterator it = _mouseMap.find(button);
    if (it != std::end(_mouseMap)) {
        btnCbksOut = it->second;
        return true;
    }

    return false;
}

bool SceneInput::addJoystickMapping(Input::JoystickButton button,
                                    const PressReleaseActions& btnCbks) {
    hashAlg::pair<JoystickMap::iterator, bool> result =
        hashAlg::emplace(_joystickMap, button, btnCbks);

    return result.second;
}

bool SceneInput::removeJoystickMapping(Input::JoystickButton button) {
    JoystickMap::const_iterator it = _joystickMap.find(button);
    if (it != std::end(_joystickMap)) {
        _joystickMap.erase(it);
        return false;
    }

    return false;
}

bool SceneInput::getJoystickMapping(Input::JoystickButton button,
                                    PressReleaseActions& btnCbksOut) const {
    JoystickMap::const_iterator it = _joystickMap.find(button);
    if (it != std::end(_joystickMap)) {
        btnCbksOut = it->second;
        return true;
    }

    return false;
}

};
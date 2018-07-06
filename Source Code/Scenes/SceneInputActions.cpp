#include "stdafx.h"

#include "Headers/SceneInputActions.h"

namespace Divide {

PressReleaseActions::PressReleaseActions()
    : PressReleaseActions(0u, 0u)
{
}

PressReleaseActions::PressReleaseActions(U16 onPressAction,
                                         U16 onReleaseAction)
    : PressReleaseActions(onPressAction, onReleaseAction, 0u, 0u)
{
}

PressReleaseActions::PressReleaseActions(U16 onPressAction,
                                         U16 onReleaseAction,
                                         U16 onLCtrlPressAction,
                                         U16 onLCtrlReleaseAction,
                                         U16 onRCtrlPressAction,
                                         U16 onRCtrlReleaseAction,
                                         U16 onLAltPressAction,
                                         U16 onLAltReleaseAction,
                                         U16 onRAltPressAction,
                                         U16 onRAltReleaseAction)
{
    _actions[to_base(Action::PRESS)] = onPressAction;
    _actions[to_base(Action::RELEASE)] = onReleaseAction;
    _actions[to_base(Action::LEFT_CTRL_PRESS)] = onLCtrlPressAction;
    _actions[to_base(Action::LEFT_CTRL_RELEASE)] = onLCtrlReleaseAction;
    _actions[to_base(Action::RIGHT_CTRL_PRESS)] = onRCtrlPressAction;
    _actions[to_base(Action::RIGHT_CTRL_RELEASE)] = onRCtrlReleaseAction;
    _actions[to_base(Action::LEFT_ALT_PRESS)] = onLAltPressAction;
    _actions[to_base(Action::LEFT_ALT_RELEASE)] = onLAltReleaseAction;
    _actions[to_base(Action::RIGHT_ALT_PRESS)] = onRAltPressAction;
    _actions[to_base(Action::RIGHT_ALT_RELEASE)] = onRAltReleaseAction;
}

InputAction::InputAction()
{
}

InputAction::InputAction(DELEGATE_CBK<void, InputParams>& action)
    : _action(action)
{
}

void InputAction::displayName(const stringImpl& name) {
    _displayName = name;
}

InputActionList::InputActionList()
{
    _noOPAction.displayName("no-op");
}

bool InputActionList::registerInputAction(U16 id, DELEGATE_CBK<void, InputParams> action) {
    if (_inputActions.find(id) == std::cend(_inputActions)) {
        hashAlg::emplace(_inputActions, id, action);
        return true;
    }

    return false;
}

InputAction& InputActionList::getInputAction(U16 id) {
    hashMap<U16, InputAction>::iterator it;
    it = _inputActions.find(id);

    if (it != std::cend(_inputActions)) {
        return it->second;
    }

    return _noOPAction;
}

const InputAction& InputActionList::getInputAction(U16 id) const {
    hashMap<U16, InputAction>::const_iterator it;
    it = _inputActions.find(id);

    if (it != std::cend(_inputActions)) {
        return it->second;
    }

    return _noOPAction;
}

}; //namespace Divide
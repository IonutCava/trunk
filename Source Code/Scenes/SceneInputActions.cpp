#include "stdafx.h"

#include "Headers/SceneInputActions.h"

namespace Divide {

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
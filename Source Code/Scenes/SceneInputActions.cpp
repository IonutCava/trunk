#include "stdafx.h"

#include "Headers/SceneInputActions.h"

namespace Divide {

namespace {
    static DELEGATE_CBK<void, InputParams> no_op = [](InputParams) {};
};

InputAction::InputAction(const DELEGATE_CBK<void, InputParams>& action)
    : _action(action)
{
}

void InputAction::displayName(const Str64& name) {
    _displayName = name;
}

InputActionList::InputActionList()
    :_noOPAction(no_op)
{
    _noOPAction.displayName("no-op");
}

bool InputActionList::registerInputAction(U16 id, const DELEGATE_CBK<void, InputParams>& action) {
    if (_inputActions.find(id) == std::cend(_inputActions)) {
        return _inputActions.emplace(id, action).second;
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
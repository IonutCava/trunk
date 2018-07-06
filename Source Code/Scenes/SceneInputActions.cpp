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
    : _onPressAction(onPressAction),
      _onReleaseAction(onReleaseAction),
      _onLCtrlPressAction(onLCtrlPressAction),
      _onLCtrlReleaseAction(onLCtrlReleaseAction),
      _onRCtrlPressAction(onRCtrlPressAction),
      _onRCtrlReleaseAction(onRCtrlReleaseAction),
      _onLAltPressAction(onLAltPressAction),
      _onLAltReleaseAction(onLAltReleaseAction),
      _onRAltPressAction(onRAltPressAction),
      _onRAltReleaseAction(onRAltReleaseAction)
{
}

InputActionList::InputActionList()
{
  
}

bool InputActionList::registerInputAction(U16 id, DELEGATE_CBK<> action) {
    if (_inputActions.find(id) != std::cend(_inputActions)) {
        hashAlg::emplace(_inputActions, id, action);
        return true;
    }

    return false;
}

DELEGATE_CBK<> InputActionList::getInputAction(U16 id) const {
    hashMapImpl<U16, DELEGATE_CBK<>>::const_iterator it;
    it = _inputActions.find(id);

    if (it != std::cend(_inputActions)) {
        return it->second;
    }

    return DELEGATE_CBK<>();
}

}; //namespace Divide
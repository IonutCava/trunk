#include "Headers/EventHandler.h"
#include "Headers/InputInterface.h"
#include "Core/Headers/Kernel.h"

namespace Divide {
namespace Input {

EventHandler::EventHandler(InputInterface *pApp, Kernel& kernel)
    : _kernel(&kernel),
      _pApplication(pApp),
      _pJoystickInterface(nullptr),
      _pEffectMgr(nullptr)
{
}

void EventHandler::initialize(JoystickInterface *pJoystickInterface,
                              EffectManager *pEffectMgr) {
    _pJoystickInterface = pJoystickInterface;
    _pEffectMgr = pEffectMgr;
}

/// Input events are either handled by the kernel
bool EventHandler::onKeyDown(const KeyEvent &arg) {
    return _kernel->onKeyDown(arg);
}

bool EventHandler::onKeyUp(const KeyEvent &arg) {
    return _kernel->onKeyUp(arg);
}

bool EventHandler::joystickButtonPressed(const JoystickEvent &arg,
                                         JoystickButton button) {
    return _kernel->joystickButtonPressed(arg, button);
}

bool EventHandler::joystickButtonReleased(const JoystickEvent &arg,
                                          JoystickButton button) {
    return _kernel->joystickButtonReleased(arg, button);
}

bool EventHandler::joystickAxisMoved(const JoystickEvent &arg, I8 axis) {
    return _kernel->joystickAxisMoved(arg, axis);
}

bool EventHandler::joystickPovMoved(const JoystickEvent &arg, I8 pov) {
    return _kernel->joystickPovMoved(arg, pov);
}

bool EventHandler::joystickSliderMoved(const JoystickEvent &arg, I8 index) {
    return _kernel->joystickSliderMoved(arg, index);
}

bool EventHandler::joystickVector3DMoved(const JoystickEvent &arg, I8 index) {
    return _kernel->joystickVector3DMoved(arg, index);
}

bool EventHandler::mouseMoved(const MouseEvent &arg) {
    return _kernel->mouseMoved(arg);
}

bool EventHandler::mouseButtonPressed(const MouseEvent &arg, MouseButton id) {
    return _kernel->mouseButtonPressed(arg, id);
}

bool EventHandler::mouseButtonReleased(const MouseEvent &arg, MouseButton id) {
    return _kernel->mouseButtonReleased(arg, id);
}

bool EventHandler::keyPressed(const OIS::KeyEvent &arg) {
    KeyEvent &key =
        Attorney::InputInterfaceEvent::getKeyRef(to_uint(arg.key));
    key._text = arg.text;
    key._pressed = true;
    return onKeyDown(key);
}

bool EventHandler::keyReleased(const OIS::KeyEvent &arg) {
    KeyEvent &key =
        Attorney::InputInterfaceEvent::getKeyRef(to_uint(arg.key));
    key._text = arg.text;
    key._pressed = false;
    return onKeyUp(key);
}

//////////// Effect variables applier functions
////////////////////////////////////////////////
// These functions apply the given Variables to the given OIS::Effect

// Variable force "Force" + optional "AttackFactor" constant, on a
// OIS::ConstantEffect
void forceVariableApplier(MapVariables &mapVars, OIS::Effect *pEffect) {
    D32 dForce = mapVars["Force"]->getValue();
    D32 dAttackFactor = 1.0;
    if (mapVars.find("AttackFactor") != std::end(mapVars))
        dAttackFactor = mapVars["AttackFactor"]->getValue();

    OIS::ConstantEffect *pConstForce =
        dynamic_cast<OIS::ConstantEffect *>(pEffect->getForceEffect());
    pConstForce->level = (I16)dForce;
    pConstForce->envelope.attackLevel = (U16)std::fabs(dForce * dAttackFactor);
    pConstForce->envelope.fadeLevel =
        (U16)std::fabs(dForce);  // Fade never reached, in fact.
}

// Variable "Period" on an OIS::PeriodicEffect
void periodVariableApplier(MapVariables &mapVars, OIS::Effect *pEffect) {
    D32 dPeriod = mapVars["Period"]->getValue();

    OIS::PeriodicEffect *pPeriodForce =
        dynamic_cast<OIS::PeriodicEffect *>(pEffect->getForceEffect());
    pPeriodForce->period = (U32)dPeriod;
}

LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

};  // namespace Input
};  // namespace Divide
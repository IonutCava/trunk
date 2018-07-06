#include "stdafx.h"

#include "Headers/EventHandler.h"
#include "Headers/InputInterface.h"
#include "Core/Headers/Kernel.h"

namespace Divide {
namespace Input {

EventHandler::EventHandler(InputInterface *pApp, InputAggregatorInterface& eventListener)
    : _eventListener(eventListener),
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
    return _eventListener.onKeyDown(arg);
}

bool EventHandler::onKeyUp(const KeyEvent &arg) {
    return _eventListener.onKeyUp(arg);
}

bool EventHandler::joystickButtonPressed(const JoystickEvent &arg, JoystickButton button) {
    return _eventListener.joystickButtonPressed(arg, button);
}

bool EventHandler::buttonPressed(const OIS::JoyStickEvent& arg, JoystickButton button) {
    return joystickButtonPressed(JoystickEvent(to_U8(arg.device->getID()), arg), button);
}

bool EventHandler::joystickButtonReleased(const JoystickEvent &arg, JoystickButton button) {
    return _eventListener.joystickButtonReleased(arg, button);
}

bool EventHandler::joystickButtonReleased(const OIS::JoyStickEvent& arg, JoystickButton button) {
    return joystickButtonReleased(JoystickEvent(to_U8(arg.device->getID()), arg), button);
}

bool EventHandler::joystickAxisMoved(const JoystickEvent &arg, I8 axis) {
    return _eventListener.joystickAxisMoved(arg, axis);
}

bool EventHandler::axisMoved(const OIS::JoyStickEvent& arg, int axis) {
    return joystickAxisMoved(JoystickEvent(to_U8(arg.device->getID()), arg), to_I8(axis));
}

bool EventHandler::joystickPovMoved(const JoystickEvent &arg, I8 pov) {
    return _eventListener.joystickPovMoved(arg, pov);
}

bool EventHandler::povMoved(const OIS::JoyStickEvent& arg, int pov) {
    return joystickPovMoved(JoystickEvent(to_U8(arg.device->getID()), arg), to_I8(pov));
}

bool EventHandler::joystickSliderMoved(const JoystickEvent &arg, I8 index) {
    return _eventListener.joystickSliderMoved(arg, index);
}

bool EventHandler::sliderMoved(const OIS::JoyStickEvent& arg, int index) {
    return joystickSliderMoved(JoystickEvent(to_U8(arg.device->getID()), arg), to_I8(index));
}

bool EventHandler::joystickvector3Moved(const JoystickEvent &arg, I8 index) {
    return _eventListener.joystickvector3Moved(arg, index);
}

bool EventHandler::vector3Moved(const OIS::JoyStickEvent& arg, int index) {
    return joystickvector3Moved(JoystickEvent(to_U8(arg.device->getID()), arg), to_I8(index));
}

bool EventHandler::mouseMoved(const MouseEvent &arg) {
    return _eventListener.mouseMoved(arg);
}

bool EventHandler::mouseMoved(const OIS::MouseEvent &arg) {
    return mouseMoved(MouseEvent(to_U8(arg.device->getID()), arg));
}

bool EventHandler::mouseButtonPressed(const MouseEvent &arg, MouseButton id) {
    return _eventListener.mouseButtonPressed(arg, id);
}

bool EventHandler::mousePressed(const OIS::MouseEvent& arg, OIS::MouseButtonID id) {
    return mouseButtonPressed(MouseEvent(to_U8(arg.device->getID()), arg), id);
}

bool EventHandler::mouseButtonReleased(const MouseEvent &arg, MouseButton id) {
    return _eventListener.mouseButtonReleased(arg, id);
}

bool EventHandler::mouseReleased(const OIS::MouseEvent& arg, OIS::MouseButtonID id) {
    return mouseButtonReleased(MouseEvent(to_U8(arg.device->getID()), arg), id);
}

bool EventHandler::keyPressed(const OIS::KeyEvent &arg) {
    KeyEvent &key =
        Attorney::InputInterfaceEvent::getKeyRef(*_pApplication, to_U32(arg.key));
    key._text = arg.text;
    key._pressed = true;
    key._deviceIndex = to_U8(arg.device->getID());
    return onKeyDown(key);
}

bool EventHandler::keyReleased(const OIS::KeyEvent &arg) {
    KeyEvent &key =
        Attorney::InputInterfaceEvent::getKeyRef(*_pApplication, to_U32(arg.key));
    key._text = arg.text;
    key._pressed = false;
    key._deviceIndex = to_U8(arg.device->getID());
    return onKeyUp(key);
}

//////////// Effect variables applier functions
////////////////////////////////////////////////
// These functions apply the given Variables to the given OIS::Effect

// Variable force "Force" + optional "AttackFactor" constant, on a
// OIS::ConstantEffect
void forceVariableApplier(MapVariables &mapVars, OIS::Effect *pEffect) {
    D64 dForce = mapVars[_ID("Force")]->getValue();
    D64 dAttackFactor = 1.0;
    if (mapVars.find(_ID("AttackFactor")) != std::end(mapVars))
        dAttackFactor = mapVars[_ID("AttackFactor")]->getValue();

    OIS::ConstantEffect *pConstForce =
        dynamic_cast<OIS::ConstantEffect *>(pEffect->getForceEffect());
    pConstForce->level = (I16)dForce;
    pConstForce->envelope.attackLevel = (U16)std::fabs(dForce * dAttackFactor);
    pConstForce->envelope.fadeLevel =
        (U16)std::fabs(dForce);  // Fade never reached, in fact.
}

// Variable "Period" on an OIS::PeriodicEffect
void periodVariableApplier(MapVariables &mapVars, OIS::Effect *pEffect) {
    D64 dPeriod = mapVars[_ID("Period")]->getValue();

    OIS::PeriodicEffect *pPeriodForce =
        dynamic_cast<OIS::PeriodicEffect *>(pEffect->getForceEffect());
    pPeriodForce->period = to_U32(dPeriod);
}

};  // namespace Input
};  // namespace Divide

#include "stdafx.h"

#include "Headers/EventHandler.h"
#include "Core/Headers/Kernel.h"

namespace Divide {
namespace Input {

EventHandler::EventHandler(InputAggregatorInterface& eventListener)
    : _eventListener(eventListener)
{
}

/// Input events are either handled by the kernel
bool EventHandler::onKeyDown(const KeyEvent &arg) {
    return _eventListener.onKeyDown(arg);
}

bool EventHandler::onKeyUp(const KeyEvent &arg) {
    return _eventListener.onKeyUp(arg);
}

bool EventHandler::joystickButtonPressed(const JoystickEvent &arg) {
    return _eventListener.joystickButtonPressed(arg);
}

bool EventHandler::joystickButtonReleased(const JoystickEvent &arg) {
    return _eventListener.joystickButtonReleased(arg);
}

bool EventHandler::joystickAxisMoved(const JoystickEvent &arg) {
    return _eventListener.joystickAxisMoved(arg);
}

bool EventHandler::joystickPovMoved(const JoystickEvent &arg) {
    return _eventListener.joystickPovMoved(arg);
}

bool EventHandler::joystickBallMoved(const JoystickEvent &arg) {
    return _eventListener.joystickBallMoved(arg);
}

bool EventHandler::joystickAddRemove(const JoystickEvent &arg) {
    return _eventListener.joystickAddRemove(arg);
}

bool EventHandler::joystickRemap(const Input::JoystickEvent &arg) {
    return _eventListener.joystickRemap(arg);
}

bool EventHandler::mouseMoved(const MouseMoveEvent &arg) {
    return _eventListener.mouseMoved(arg);
}

bool EventHandler::mouseButtonPressed(const MouseButtonEvent &arg) {
    return _eventListener.mouseButtonPressed(arg);
}

bool EventHandler::mouseButtonReleased(const MouseButtonEvent &arg) {
    return _eventListener.mouseButtonReleased(arg);
}

/// UTF8 text input event - Engine: return true if input was consumed
bool EventHandler::onUTF8(const Input::UTF8Event& arg) {
    return _eventListener.onUTF8(arg);
}

};  // namespace Input
};  // namespace Divide

#include "Headers/EventHandler.h"
#include "Headers/InputInterface.h"
#include "Core/Headers/Kernel.h"

EventHandler::EventHandler(InputInterface* pApp, Kernel* const kernel) :
												   _kernel(kernel),
												   _pApplication(pApp),
												   _pJoystickInterface(nullptr),
												   _pEffectMgr(nullptr)
{
	DIVIDE_ASSERT(kernel != nullptr, "EventHandler error: INVALID KERNEL PASSED ON HANDLER CREATION");
}

void EventHandler::initialize(JoystickInterface* pJoystickInterface, EffectManager* pEffectMgr){
  _pJoystickInterface = pJoystickInterface;
  _pEffectMgr = pEffectMgr;
}

/// Input events are either handled by the kernel
bool EventHandler::onKeyDown( const OIS::KeyEvent &arg ) {
    return _kernel->onKeyDown(arg); 
}

bool EventHandler::onKeyUp( const OIS::KeyEvent &arg ) {
    return _kernel->onKeyUp(arg); 
}

bool EventHandler::joystickButtonPressed( const OIS::JoyStickEvent &arg, I8 button ) {
    return _kernel->joystickButtonPressed(arg,button); 
}

bool EventHandler::joystickButtonReleased( const OIS::JoyStickEvent &arg, I8 button ) {
    return _kernel->joystickButtonReleased(arg,button); 
}

bool EventHandler::joystickAxisMoved( const OIS::JoyStickEvent &arg, I8 axis) {
    return _kernel->joystickAxisMoved(arg,axis); 
}

bool EventHandler::joystickPovMoved( const OIS::JoyStickEvent &arg, I8 pov ) {
    return _kernel->joystickPovMoved(arg,pov); 
}

bool EventHandler::joystickSliderMoved( const OIS::JoyStickEvent &arg, I8 index) {
    return _kernel->joystickSliderMoved(arg,index); 
}

bool EventHandler::joystickVector3DMoved( const OIS::JoyStickEvent &arg, I8 index) {
    return _kernel->joystickVector3DMoved(arg,index); 
}

bool EventHandler::mouseMoved( const OIS::MouseEvent &arg ) {
    return _kernel->mouseMoved(arg);
}

bool EventHandler::mouseButtonPressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id ) {
    return _kernel->mouseButtonPressed(arg,id); 
}

bool EventHandler::mouseButtonReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id ) {
    return _kernel->mouseButtonReleased(arg,id); 
}

//////////// Effect variables applier functions /////////////////////////////////////////////
// These functions apply the given Variables to the given OIS::Effect

// Variable force "Force" + optional "AttackFactor" constant, on a OIS::ConstantEffect
void forceVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect){
  D32 dForce = mapVars["Force"]->getValue();
  D32 dAttackFactor = 1.0;
  if (mapVars.find("AttackFactor") != mapVars.end())
	dAttackFactor = mapVars["AttackFactor"]->getValue();

  OIS::ConstantEffect* pConstForce = dynamic_cast<OIS::ConstantEffect*>(pEffect->getForceEffect());
  pConstForce->level = (I16)dForce;
  pConstForce->envelope.attackLevel = (U16)fabs(dForce*dAttackFactor);
  pConstForce->envelope.fadeLevel = (U16)fabs(dForce); // Fade never reached, in fact.
}

// Variable "Period" on an OIS::PeriodicEffect
void periodVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect){
  D32 dPeriod = mapVars["Period"]->getValue();

  OIS::PeriodicEffect* pPeriodForce = dynamic_cast<OIS::PeriodicEffect*>(pEffect->getForceEffect());
  pPeriodForce->period = (U32)dPeriod;
}

LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ){	return FALSE; }
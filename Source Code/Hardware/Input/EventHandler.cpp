#include "Headers/EventHandler.h"
#include "Headers/InputInterface.h"
#include "Core/Headers/Kernel.h"

static I32 axisDeadZone = 256;

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
bool EventHandler::keyPressed( const OIS::KeyEvent &arg )                    {return _kernel->onKeyDown(arg); }
bool EventHandler::keyReleased( const OIS::KeyEvent &arg )                   {return _kernel->onKeyUp(arg); }
bool EventHandler::buttonPressed( const OIS::JoyStickEvent &arg, I8 button ) {return _kernel->onJoystickButtonDown(arg,button); }
bool EventHandler::buttonReleased( const OIS::JoyStickEvent &arg, I8 button ){return _kernel->onJoystickButtonUp(arg,button); }
bool EventHandler::axisMoved( const OIS::JoyStickEvent &arg, I8 axis)        {return _kernel->onJoystickMoveAxis(arg,axis,axisDeadZone); }
bool EventHandler::povMoved( const OIS::JoyStickEvent &arg, I8 pov )         {return _kernel->onJoystickMovePOV(arg,pov); }
bool EventHandler::sliderMoved( const OIS::JoyStickEvent &arg, I8 index)     {return _kernel->sliderMoved(arg,index); }
bool EventHandler::vector3Moved( const OIS::JoyStickEvent &arg, I8 index)    {return _kernel->vector3Moved(arg,index); }
bool EventHandler::mouseMoved( const OIS::MouseEvent &arg )                            {return _kernel->onMouseMove(arg); }
bool EventHandler::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )   {return _kernel->onMouseClickDown(arg,id); }
bool EventHandler::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )  {return _kernel->onMouseClickUp(arg,id); }

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
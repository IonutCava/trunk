#include "EventHandler.h"
#include "InputManager.h"
#include "Managers/SceneManager.h"

using namespace OIS;

EventHandler::EventHandler(InputManagerInterface* pApp)
: _pApplication(pApp)
{
	_activeScene = SceneManager::getInstance().getActiveScene();
}

void EventHandler::initialize(JoystickManager* pJoystickMgr, EffectManager* pEffectMgr)
{

  _pJoystickMgr = pJoystickMgr;
  _pEffectMgr = pEffectMgr;
}

bool EventHandler::keyPressed( const OIS::KeyEvent &arg )
{
	_activeScene->onKeyDown(arg);
    return true;
}

bool EventHandler::keyReleased( const OIS::KeyEvent &arg )
{
	_activeScene->onKeyUp(arg);
	return true;
}

bool EventHandler::buttonPressed( const OIS::JoyStickEvent &arg, int button )
{
  _activeScene->OnJoystickButtonDown(arg,button);
  return true;
}

bool EventHandler::buttonReleased( const OIS::JoyStickEvent &arg, int button )
{
  _activeScene->OnJoystickButtonUp(arg,button);
  return true;
}

bool EventHandler::axisMoved( const OIS::JoyStickEvent &arg, int axis )
{
  _activeScene->OnJoystickMoveAxis(arg,axis);
  return true;
}

bool EventHandler::povMoved( const OIS::JoyStickEvent &arg, int pov )
{
  _activeScene->OnJoystickMovePOV(arg,pov);
  return true;
}

bool EventHandler::mouseMoved( const MouseEvent &arg )
{
	const OIS::MouseState& s = arg.state;
	_activeScene->onMouseMove(arg);
	return true;
}

bool EventHandler::mousePressed( const MouseEvent &arg, MouseButtonID id )
{
	const OIS::MouseState& s = arg.state;
	_activeScene->onMouseClickDown(arg,id);
	return true;
}

bool EventHandler::mouseReleased( const MouseEvent &arg, MouseButtonID id )
{
	const OIS::MouseState& s = arg.state;
	_activeScene->onMouseClickUp(arg,id);
	return true;
}

//////////// Effect variables applier functions /////////////////////////////////////////////
// These functions apply the given Variables to the given OIS::Effect

// Variable force "Force" + optional "AttackFactor" constant, on a OIS::ConstantEffect
void forceVariableApplier(MapVariables& mapVars, Effect* pEffect)
{
  double dForce = mapVars["Force"]->getValue();
  double dAttackFactor = 1.0;
  if (mapVars.find("AttackFactor") != mapVars.end())
	dAttackFactor = mapVars["AttackFactor"]->getValue();

  ConstantEffect* pConstForce = dynamic_cast<ConstantEffect*>(pEffect->getForceEffect());
  pConstForce->level = (int)dForce;
  pConstForce->envelope.attackLevel = (unsigned short)fabs(dForce*dAttackFactor);
  pConstForce->envelope.fadeLevel = (unsigned short)fabs(dForce); // Fade never reached, in fact.
}

// Variable "Period" on an OIS::PeriodicEffect
void periodVariableApplier(MapVariables& mapVars, Effect* pEffect)
{
  double dPeriod = mapVars["Period"]->getValue();

  PeriodicEffect* pPeriodForce = dynamic_cast<PeriodicEffect*>(pEffect->getForceEffect());
  pPeriodForce->period = (unsigned int)dPeriod;
}


LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return FALSE;
}

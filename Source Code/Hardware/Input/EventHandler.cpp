#include "EventHandler.h"
#include "InputManager.h"
#include "Managers/Headers/SceneManager.h"

using namespace OIS;

EventHandler::EventHandler(InputManagerInterface* pApp)
: _pApplication(pApp), _pJoystickMgr(NULL), _pEffectMgr(NULL)
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
	//boost::mutex::scoped_lock l(_mutex);
	_activeScene->onKeyDown(arg);
    return true;
}

bool EventHandler::keyReleased( const OIS::KeyEvent &arg )
{
	//boost::mutex::scoped_lock l(_mutex);
	_activeScene->onKeyUp(arg);
	return true;
}

bool EventHandler::buttonPressed( const OIS::JoyStickEvent &arg, I8 button )
{
  //boost::mutex::scoped_lock l(_mutex);
  _activeScene->OnJoystickButtonDown(arg,button);
  return true;
}

bool EventHandler::buttonReleased( const OIS::JoyStickEvent &arg, I8 button )
{
  //boost::mutex::scoped_lock l(_mutex);
  _activeScene->OnJoystickButtonUp(arg,button);
  return true;
}

bool EventHandler::axisMoved( const OIS::JoyStickEvent &arg, I8 axis )
{
   //boost::mutex::scoped_lock l(_mutex);
  _activeScene->OnJoystickMoveAxis(arg,axis);
  return true;
}

bool EventHandler::povMoved( const OIS::JoyStickEvent &arg, I8 pov )
{
   //boost::mutex::scoped_lock l(_mutex);
  _activeScene->OnJoystickMovePOV(arg,pov);
  return true;
}

bool EventHandler::mouseMoved( const MouseEvent &arg )
{
	//boost::mutex::scoped_lock l(_mutex);
	const OIS::MouseState& s = arg.state;
	_activeScene->onMouseMove(arg);
	return true;
}

bool EventHandler::mousePressed( const MouseEvent &arg, MouseButtonID id )
{
	//boost::mutex::scoped_lock l(_mutex);
	const OIS::MouseState& s = arg.state;
	_activeScene->onMouseClickDown(arg,id);
	return true;
}

bool EventHandler::mouseReleased( const MouseEvent &arg, MouseButtonID id )
{
	//boost::mutex::scoped_lock l(_mutex);
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
  pConstForce->level = (I16)dForce;
  pConstForce->envelope.attackLevel = (U16)fabs(dForce*dAttackFactor);
  pConstForce->envelope.fadeLevel = (U16)fabs(dForce); // Fade never reached, in fact.
}

// Variable "Period" on an OIS::PeriodicEffect
void periodVariableApplier(MapVariables& mapVars, Effect* pEffect)
{
  double dPeriod = mapVars["Period"]->getValue();

  PeriodicEffect* pPeriodForce = dynamic_cast<PeriodicEffect*>(pEffect->getForceEffect());
  pPeriodForce->period = (U32)dPeriod;
}


LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return FALSE;
}

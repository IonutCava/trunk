/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "JoystickManager.h"
#include "Core/Headers/Application.h"

#if defined OIS_WIN32_PLATFORM
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include "windows.h"
#  ifdef min
#    undef min
#  endif
LRESULT DlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Linux Headers//////////////
#elif defined OIS_LINUX_PLATFORM
#  include <X11/Xlib.h>
   void checkX11Events();
//////////////////////////////////////////////////////////////////////
////////////////////////////////////Needed Mac Headers//////////////
#elif defined OIS_APPLE_PLATFORM
#  include <Carbon/Carbon.h>
   void checkMacEvents();
#endif

//////////// Event handler class declaration ////////////////////////////////////////////////
class InputManagerInterface;
class JoystickManager;
class EffectManager;

void forceVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);
void periodVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);
//////////// Effect manager class //////////////////////////////////////////////////////////

class EffectManager{
  protected:

    // The joystick manager
    JoystickManager* _pJoystickMgr;

    // Vector to hold variable effects
    std::vector<VariableEffect*> _vecEffects;

    // Selected effect
    I8 _nCurrEffectInd;

    // Update frequency (Hz)
    U32 _nUpdateFreq;

	// Indexes (in _vecEffects) of the variable effects that are playable by the selected joystick.
	std::vector<size_t> _vecPlayableEffectInd;


  public:

    EffectManager(JoystickManager* pJoystickMgr, U32 nUpdateFreq) 
	: _pJoystickMgr(pJoystickMgr), _nUpdateFreq(nUpdateFreq), _nCurrEffectInd(-1)
    {
	  OIS::Effect* pEffect;
	  MapVariables mapVars;
	  OIS::ConstantEffect* pConstForce;
	  OIS::PeriodicEffect* pPeriodForce;

	  pEffect = new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
	  pEffect->direction = OIS::Effect::North;
	  pEffect->trigger_button = 0;
	  pEffect->trigger_interval = 0;
	  pEffect->replay_length = OIS::Effect::OIS_INFINITE; // Linux/Win32: Same behaviour as 0.
	  pEffect->replay_delay = 0;
	  pEffect->setNumAxes(1);
	  pConstForce = dynamic_cast<OIS::ConstantEffect*>(pEffect->getForceEffect());
	  pConstForce->level = 5000;  //-10K to +10k
	  pConstForce->envelope.attackLength = 0;
	  pConstForce->envelope.attackLevel = (unsigned short)pConstForce->level;
	  pConstForce->envelope.fadeLength = 0;
	  pConstForce->envelope.fadeLevel = (unsigned short)pConstForce->level;

	  mapVars.clear();
	  mapVars["Force"] = 
		new TriangleVariable(0.0, // F0
							 4*10000/_nUpdateFreq / 20.0, // dF for a 20s-period triangle
							 -10000.0, // Fmin 
							 10000.0); // Fmax
	  mapVars["AttackFactor"] = new Constant(1.0);

	  _vecEffects.push_back
		(new VariableEffect
		       ("Constant force on 1 axis with 20s-period triangle oscillations "
				"of its signed amplitude in [-10K, +10K]",
				pEffect, mapVars, forceVariableApplier));

	  pEffect = new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
	  pEffect->direction = OIS::Effect::North;
	  pEffect->trigger_button = 0;
	  pEffect->trigger_interval = 0;
	  pEffect->replay_length = OIS::Effect::OIS_INFINITE; //(U32)(1000000.0/_nUpdateFreq); // Linux: Does not work.
	  pEffect->replay_delay = 0;
	  pEffect->setNumAxes(1);
	  pConstForce = dynamic_cast<OIS::ConstantEffect*>(pEffect->getForceEffect());
	  pConstForce->level = 5000;  //-10K to +10k
	  pConstForce->envelope.attackLength = (U32)(1000000.0/_nUpdateFreq/2);
	  pConstForce->envelope.attackLevel = (U16)(pConstForce->level*0.1);
	  pConstForce->envelope.fadeLength = 0; // Never reached, actually.
	  pConstForce->envelope.fadeLevel = (U16)pConstForce->level; // Idem

	  mapVars.clear();
	  mapVars["Force"] = 
		new TriangleVariable(0.0, // F0
							 4*10000/_nUpdateFreq / 20.0, // dF for a 20s-period triangle
							 -10000.0, // Fmin 
							 10000.0); // Fmax
	  mapVars["AttackFactor"] = new Constant(0.1);

	  _vecEffects.push_back
		(new VariableEffect
		       ("Constant force on 1 axis with noticeable attack (app update period / 2)"
				"and 20s-period triangle oscillations of its signed amplitude in [-10K, +10K]",
				pEffect, mapVars, forceVariableApplier));

	  pEffect = new OIS::Effect(OIS::Effect::PeriodicForce, OIS::Effect::Triangle);
	  pEffect->direction = OIS::Effect::North;
	  pEffect->trigger_button = 0;
	  pEffect->trigger_interval = 0;
	  pEffect->replay_length = OIS::Effect::OIS_INFINITE;
	  pEffect->replay_delay = 0;
	  pEffect->setNumAxes(1);
	  pPeriodForce = dynamic_cast<OIS::PeriodicEffect*>(pEffect->getForceEffect());
	  pPeriodForce->magnitude = 10000;  // 0 to +10k
	  pPeriodForce->offset = 0;
	  pPeriodForce->phase = 0;  // 0 to 35599
	  pPeriodForce->period = 10000;  // Micro-seconds
	  pPeriodForce->envelope.attackLength = 0;
	  pPeriodForce->envelope.attackLevel = (unsigned short)pPeriodForce->magnitude;
	  pPeriodForce->envelope.fadeLength = 0;
	  pPeriodForce->envelope.fadeLevel = (unsigned short)pPeriodForce->magnitude;

	  mapVars.clear();
	  mapVars["Period"] = 
		new TriangleVariable(1*1000.0, // P0
							 4*(400-10)*1000.0/_nUpdateFreq / 40.0, // dP for a 40s-period triangle
							 10*1000.0, // Pmin 
							 400*1000.0); // Pmax
	  _vecEffects.push_back
		(new VariableEffect
		       ("Periodic force on 1 axis with 40s-period triangle oscillations "
				"of its period in [10, 400] ms, and constant amplitude",
				pEffect, mapVars, periodVariableApplier));

	}

    ~EffectManager()
    {
	  std::vector<VariableEffect*>::iterator iterEffs;
	  for (iterEffs = _vecEffects.begin(); iterEffs != _vecEffects.end(); ++iterEffs)
		   SAFE_DELETE(*iterEffs);
	  }

    void updateActiveEffects()
    {
	  std::vector<VariableEffect*>::iterator iterEffs;
	  for (iterEffs = _vecEffects.begin(); iterEffs != _vecEffects.end(); ++iterEffs)
		if ((*iterEffs)->isActive())
		{
		  (*iterEffs)->update();
		  _pJoystickMgr->getCurrentFFDevice()->modify((*iterEffs)->getFFEffect());
		}
	}

    void checkPlayableEffects()
    {
	  // Nothing to do if no joystick currently selected
	  if (!_pJoystickMgr->getCurrentFFDevice())
		return;

	  // Get the list of indexes of effects that the selected device can play
	  _vecPlayableEffectInd.clear();
	  for (size_t nEffInd = 0; nEffInd < _vecEffects.size(); nEffInd++)
	  {
		const OIS::Effect::EForce eForce = _vecEffects[nEffInd]->getFFEffect()->force;
		const OIS::Effect::EType eType = _vecEffects[nEffInd]->getFFEffect()->type;
		if (_pJoystickMgr->getCurrentFFDevice()->supportsEffect(eForce, eType))
		{
		  _vecPlayableEffectInd.push_back(nEffInd);
		}
	  }

	  // Print details about playable effects
	  if (_vecPlayableEffectInd.empty())
	  {
		  D_PRINT_FN("InputManager: The device can't play any effect of the test set");
	  }
	  else
	  {
		  PRINT_FN("InputManager: Selected device can play the following effects :");
		for (size_t nEffIndInd = 0; nEffIndInd < _vecPlayableEffectInd.size(); nEffIndInd++)
			printEffect(_vecPlayableEffectInd[nEffIndInd]);
		PRINT_FN("");
	  }
	}

    enum EWhichEffect { ePrevious=-1, eNone=0, eNext=+1 };

    void selectEffect(EWhichEffect eWhich)
    {

	  // Nothing to do if no joystick currently selected
	  if (!_pJoystickMgr->getCurrentFFDevice())
	  {
		  D_PRINT_FN("InputManager: No Joystick selected.");  
		return;
	  }

	  // Nothing to do if joystick cannot play any effect
	  if (_vecPlayableEffectInd.empty())
	  {
		  D_PRINT_FN("InputManager: No playable effects."); 
		return;
	  }

	  // If no effect selected, and next or previous requested, select the first one.
	  if (eWhich != eNone && _nCurrEffectInd < 0)
		_nCurrEffectInd = 0;

	  // Otherwise, remove the current one from the device, 
	  // and then select the requested one if any.
	  else if (_nCurrEffectInd >= 0)
	  {
		_pJoystickMgr->getCurrentFFDevice()
		  ->remove(_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->getFFEffect());
		_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->setActive(false);
		_nCurrEffectInd += eWhich;
		if (_nCurrEffectInd < -1 || _nCurrEffectInd >= (I16)_vecPlayableEffectInd.size())
		  _nCurrEffectInd = -1;
	  }

	  // If no effect must be selected, reset the selection index
	  if (eWhich == eNone)
	  {
		_nCurrEffectInd = -1;
	  }

	  // Otherwise, upload the new selected effect to the device if any.
	  else if (_nCurrEffectInd >= 0)
	  {
		_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->setActive(true);
		_pJoystickMgr->getCurrentFFDevice()
		  ->upload(_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->getFFEffect());
	  }
	}

    inline void printEffect(size_t nEffInd){

		PRINT_FN("InputManager: * #%d : %s",nEffInd,_vecEffects[nEffInd]->getDescription());
	}

    inline void printEffects(){

		for (size_t nEffInd = 0; nEffInd < _vecEffects.size(); nEffInd++){
		  printEffect(nEffInd);
		}
	}

   
};


DEFINE_SINGLETON( InputManagerInterface )

  protected:
    OIS::InputManager* _pInputMgr;
    EventHandler*      _pEventHdlr;
    OIS::Keyboard*     _pKeyboard;
	OIS::Mouse*        _pMouse;
	OIS::JoyStick*	   _pJoystick;
    JoystickManager*   _pJoystickMgr;
	EffectManager*     _pEffectMgr;

    bool               _bMustStop;
    bool               _bIsInitialized;

    I16 _nStatus;

    // App. hart beat frequency.
    static const U8 _nHartBeatFreq = 20; // Hz

    // Effects update frequency (Hz) : Needs to be quite lower than app. hart beat frequency,
	// if we want to be able to calmly study effect changes ...
    static const U8 _nEffectUpdateFreq = 1; // Hz

  

    InputManagerInterface()  {
	  _pInputMgr = NULL;
	  _pEventHdlr = NULL;
	  _pKeyboard = NULL;
	  _pJoystick = NULL;
	  _pJoystickMgr = NULL;
	  _pMouse = NULL;
	  _pEffectMgr = NULL;

	  _bMustStop = false;

	  _bIsInitialized = false;
	  _nStatus = 0;
	}

public:
    U8 initialize()
    {
		if(_bIsInitialized)
			return 0;
		OIS::ParamList pl;
#if defined OIS_WIN32_PLATFORM
	  // Create OIS input manager
		HWND hWnd = 0;
		hWnd = FindWindow("FREEGLUT","DIVIDE Framework");
		std::ostringstream wnd;
		wnd << (size_t)hWnd;
		pl.insert(std::make_pair(std::string("WINDOW"), wnd.str() ));
		pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
		pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
		pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
		pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
		pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
		pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
		pl.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
		pl.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#endif

	  _pInputMgr = OIS::InputManager::createInputSystem(pl);
	  PRINT_FN("InputManager: %s created.",_pInputMgr->inputSystemName().c_str());

	  // Create the event handler.
	  _pEventHdlr = new EventHandler(this);

	  try{
		// Create a simple keyboard
		_pKeyboard = (OIS::Keyboard*)_pInputMgr->createInputObject( OIS::OISKeyboard, true );
		_pKeyboard->setEventCallback( _pEventHdlr );
	  }
	  catch(OIS::Exception &ex)
	  {
		PRINT_F("Exception raised on keyboard creation: %s\n",ex.eText);
	  }

	  try{
			_pJoystick = (OIS::JoyStick*) _pInputMgr->createInputObject(OIS::OISJoyStick, true);
			if(_pJoystick){
				_pJoystick->setEventCallback( _pEventHdlr );
			}
	  }
	  catch(OIS::Exception &ex)
	  {
			PRINT_F("Exception raised on joystick creation: %s\n",ex.eText);
	  }

	  try{
	  
		_pMouse = (OIS::Mouse*)_pInputMgr->createInputObject(OIS::OISMouse,true);
		_pMouse->setEventCallback( _pEventHdlr );
		const OIS::MouseState &ms = _pMouse->getMouseState();
		ms.width = Application::getInstance().getWindowDimensions().width;
		ms.height = Application::getInstance().getWindowDimensions().height;
	  }
	  catch(OIS::Exception &ex)
	  {
		PRINT_F("Exception raised on mouse creation: %s\n",ex.eText);
	  }
	  
	  // Create the joystick manager.
	  _pJoystickMgr = new JoystickManager(_pInputMgr, _pEventHdlr);
	  if( !_pJoystickMgr->wasFFDetected() )
	  {
		PRINT_F("InputManager: No Force Feedback device detected.\n");
		SAFE_DELETE(_pJoystickMgr);
	  }
	  else
	  {	  // Create force feedback effect manager.
	      _pEffectMgr = new EffectManager(_pJoystickMgr, _nEffectUpdateFreq);
		  // Initialize the event handler.
		  _pEventHdlr->initialize(_pJoystickMgr, _pEffectMgr);
	  }

	  _bIsInitialized = true;

	  return _nStatus;
	}

#if defined OIS_LINUX_PLATFORM

    // This is just here to show that you still receive x11 events, 
    // as the lib only needs mouse/key events
    void checkX11Events()
    {
	  XEvent event;
	  
	  //Poll x11 for events
	  while( XPending(_pXDisp) > 0 )
	  {
		XNextEvent(_pXDisp, &event);
	  }
	}
#endif

    U8 tick()
    {
	  const U8 nMaxEffectUpdateCnt = _nHartBeatFreq / _nEffectUpdateFreq;
	  U8 nEffectUpdateCnt = 0;

	  if (!_bIsInitialized)	initialize();

	  if (!_bIsInitialized)
		return _nStatus;

	  try
	  {
		  // This fires off buffered events for keyboards
		  if(_pKeyboard != NULL)
			_pKeyboard->capture();
		  if(_pJoystick != NULL)
			_pJoystick->capture();
		  if(_pMouse != NULL)
			_pMouse->capture();

		  // This fires off buffered events for each joystick we have
		  if(_pJoystickMgr != NULL)
		  {
			_pJoystickMgr->captureEvents();
	        // Update currently selected effects if time has come to.
			if (!nEffectUpdateCnt)
			{
				_pEffectMgr->updateActiveEffects();
				nEffectUpdateCnt = nMaxEffectUpdateCnt;
			}
			else
				nEffectUpdateCnt--;
		  }


	  }
	  catch( ... )
	  {
	  }

	  return _nStatus;
	}

    inline void stop(){ _bMustStop = true; }

    void terminate()
    {
	  if (_pInputMgr)
	  {
		_pInputMgr->destroyInputObject( _pKeyboard );
		_pInputMgr->destroyInputObject( _pMouse );
		_pInputMgr->destroyInputObject( _pJoystick );

		_pKeyboard = NULL;
		_pMouse = NULL;
		_pJoystick = NULL;

		SAFE_DELETE(_pJoystickMgr);

		OIS::InputManager::destroyInputSystem(_pInputMgr);
		_pInputMgr = NULL;
	  }
	 
	  SAFE_DELETE(_pEffectMgr);
	  SAFE_DELETE(_pEventHdlr);

#if defined OIS_LINUX_PLATFORM
	  // Be nice to X and clean up the x window
	  XDestroyWindow(_pXDisp, _xWin);
	  XCloseDisplay(_pXDisp);
#endif
	}

    inline JoystickManager* getJoystickManager(){ return _pJoystickMgr; }
    inline EffectManager* getEffectManager(){ return _pEffectMgr; }


END_SINGLETON


#endif
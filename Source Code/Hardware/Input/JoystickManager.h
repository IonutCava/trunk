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

#ifndef JOYSTICK_MANAGER_H_
#define JOYSTICK_MANAGER_H_

#include "InputVariables.h"

//////////// Joystick manager class ////////////////////////////////////////////////////////
class EventHandler;
class JoystickManager
{
  protected:

    // Input manager.
    OIS::InputManager* _pInputMgr;

    // Vectors to hold joysticks and associated force feedback devices
    std::vector<OIS::JoyStick*> _vecJoys;
    std::vector<OIS::ForceFeedback*> _vecFFDev;

    // Selected joystick
    U8 _nCurrJoyInd;

    // Force feedback detected ?
    bool _bFFFound;

    // Selected joystick master gain.
    float _dMasterGain;

    // Selected joystick auto-center mode.
    bool _bAutoCenter;

  public:

    JoystickManager(OIS::InputManager* pInputMgr, EventHandler* pEventHdlr)
	: _pInputMgr(pInputMgr), _nCurrJoyInd(-1), _dMasterGain(0.5), _bAutoCenter(true)

    {
	  _bFFFound = false;
	  for( U8 nJoyInd = 0; nJoyInd < pInputMgr->getNumberOfDevices(OIS::OISJoyStick); ++nJoyInd ) 
	  {
		//Create the stick
		OIS::JoyStick* pJoy = (OIS::JoyStick*)pInputMgr->createInputObject( OIS::OISJoyStick, true );
		Console::getInstance().printfn("Created buffered joystick # %d '%s' (Id='%d')",nJoyInd,pJoy->vendor(),pJoy->getID());
		
		// Check for FF, and if so, keep the joy and dump FF info
		OIS::ForceFeedback* pFFDev = (OIS::ForceFeedback*)pJoy->queryInterface(OIS::Interface::ForceFeedback );
		if( pFFDev )
		{
		  _bFFFound = true;

		  // Keep the joy to play with it.
		  pJoy->setEventCallback(pEventHdlr);
		  _vecJoys.push_back(pJoy);

		  // Keep also the associated FF device
		  _vecFFDev.push_back(pFFDev);
		  
		  // Dump FF supported effects and other info.
		  Console::getInstance().printfn(" * Number of force feedback axes : %d", pFFDev->getFFAxesNumber());
		  const OIS::ForceFeedback::SupportedEffectList &lstFFEffects = 
			pFFDev->getSupportedEffects();
		  if (lstFFEffects.size() > 0)
		  {
			  Console::getInstance().printf(" * Supported effects :");
			OIS::ForceFeedback::SupportedEffectList::const_iterator itFFEff;
			for(itFFEff = lstFFEffects.begin(); itFFEff != lstFFEffects.end(); ++itFFEff)
				Console::getInstance().printf(" %s",OIS::Effect::getEffectTypeName(itFFEff->second));
			Console::getInstance().printf("\n");//Double new line - Ionut
		  }
		  else
			  Console::getInstance().d_printfn("Warning: no supported effect found !");
		}
		/*else
		{
		  cout << " (no force feedback support detected) => ignored." << endl << endl;
		  _pInputMgr->destroyInputObject(pJoy);
		}*/
	  }
	}

    ~JoystickManager()
    {
	  for(size_t nJoyInd = 0; nJoyInd < _vecJoys.size(); ++nJoyInd)
		_pInputMgr->destroyInputObject( _vecJoys[nJoyInd] );
	}

    size_t getNumberOfJoysticks() const
    {
	  return _vecJoys.size();
	}

    bool wasFFDetected() const
    {
	  return _bFFFound;
	}

	enum EWhichJoystick { ePrevious=-1, eNext=+1 };

    void selectJoystick(EWhichJoystick eWhich)
    {
	  // Note: Reset the master gain to half the maximum and autocenter mode to Off,
	  // when really selecting a new joystick.
	  if (_nCurrJoyInd < 0)
	  {
		_nCurrJoyInd = 0;
		_dMasterGain = 0.5; // Half the maximum.
		changeMasterGain(0.0);
	  }
	  else
	  {
		_nCurrJoyInd += eWhich;
		if (_nCurrJoyInd < -1 || _nCurrJoyInd >= (U8)_vecJoys.size())
		  _nCurrJoyInd = -1;
		if (_vecJoys.size() > 1 && _nCurrJoyInd >= 0)
		{
		  _dMasterGain = 0.5; // Half the maximum.
		  changeMasterGain(0.0);
		}
	  }
	}

    OIS::ForceFeedback* getCurrentFFDevice()
    {
	  return (_nCurrJoyInd >= 0) ? _vecFFDev[_nCurrJoyInd] : 0;
	}

    void changeMasterGain(float dDeltaPercent)
    {
	  if (_nCurrJoyInd >= 0)
	  {
		_dMasterGain += dDeltaPercent / 100;
		if (_dMasterGain > 1.0)
		  _dMasterGain = 1.0;
		else if (_dMasterGain < 0.0)
		  _dMasterGain = 0.0;
		
		_vecFFDev[_nCurrJoyInd]->setMasterGain(_dMasterGain);
	  }
	}

    enum EAutoCenterHow { eOff, eOn, eToggle };

    void changeAutoCenter(EAutoCenterHow eHow = eToggle)
    {
	  if (_nCurrJoyInd >= 0)
	  {
		if (eHow == eToggle)
		  _bAutoCenter = !_bAutoCenter;
		else
		  _bAutoCenter = (eHow == eOn ? true : false);
		_vecFFDev[_nCurrJoyInd]->setAutoCenterMode(_bAutoCenter);
	  }
	}

    void captureEvents()
    {
	  // This fires off buffered events for each joystick we have
	  for(size_t nJoyInd = 0; nJoyInd < _vecJoys.size(); ++nJoyInd)
		if( _vecJoys[nJoyInd] )	
		  _vecJoys[nJoyInd]->capture();
	}

};

#endif
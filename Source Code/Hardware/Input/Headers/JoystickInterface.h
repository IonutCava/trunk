/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef JOYSTICK_MANAGER_H_
#define JOYSTICK_MANAGER_H_

#include "InputVariables.h"
namespace Divide {
    namespace Input {
//////////// Joystick manager class ////////////////////////////////////////////////////////
class EventHandler;
class JoystickInterface {
protected:

    // Input manager.
    OIS::InputManager* _pInputInterface;
    // vectors to hold joysticks and associated force feedback devices
    vectorImpl<OIS::JoyStick*>      _vecJoys;
    vectorImpl<OIS::ForceFeedback*> _vecFFDev;
    // Selected joystick
    I8 _nCurrJoyInd;
    // Force feedback detected ?
    bool _bFFFound;
    // Selected joystick master gain.
    F32 _dMasterGain;
    // Selected joystick auto-center mode.
    bool _bAutoCenter;

public:
        
    JoystickInterface(OIS::InputManager* pInputInterface, EventHandler* pEventHdlr) : _pInputInterface(pInputInterface),
                                                                                        _nCurrJoyInd(-1),
                                                                                        _dMasterGain(0.5),
                                                                                        _bAutoCenter(true)  
    {
        _bFFFound = false;
        for( U8 nJoyInd = 0; nJoyInd < _pInputInterface->getNumberOfDevices(OIS::OISJoyStick); ++nJoyInd )  {
            OIS::JoyStick* pJoy = nullptr;
            OIS::ForceFeedback* pFFDev = nullptr;
                
            try{
                pJoy = static_cast<OIS::JoyStick*>(_pInputInterface->createInputObject( OIS::OISJoyStick, true ));
                if(pJoy){
                    PRINT_FN(Locale::get("INPUT_CREATE_BUFF_JOY"), 
                                nJoyInd,
                                pJoy->vendor().empty() ? "unknown vendor" : pJoy->vendor().c_str(),
                                pJoy->getID());
                    // Check for FF, and if so, keep the joy and dump FF info
                    pFFDev = (OIS::ForceFeedback*)pJoy->queryInterface(OIS::Interface::ForceFeedback );
                }
            }catch(OIS::Exception &ex){
                PRINT_FN(Locale::get("ERROR_INPUT_CREATE_JOYSTICK"),ex.eText);
            }

            if( pFFDev ) {
                _bFFFound = true;
                // Keep the joy to play with it.
                pJoy->setEventCallback(pEventHdlr);
                _vecJoys.push_back(pJoy);
                // Keep also the associated FF device
                _vecFFDev.push_back(pFFDev);
                // Dump FF supported effects and other info.
                PRINT_FN(Locale::get("INPUT_JOY_NUM_FF_AXES"), pFFDev->getFFAxesNumber());
                const OIS::ForceFeedback::SupportedEffectList &lstFFEffects = pFFDev->getSupportedEffects();
                 
                if (lstFFEffects.size() > 0) {
                    PRINT_F(Locale::get("INPUT_JOY_SUPPORTED_EFFECTS"));
                    OIS::ForceFeedback::SupportedEffectList::const_iterator itFFEff;
                    for(itFFEff = lstFFEffects.begin(); itFFEff != lstFFEffects.end(); ++itFFEff)
                        PRINT_F(" %s\n",OIS::Effect::getEffectTypeName(itFFEff->second));
                }else{
                    D_PRINT_FN(Locale::get("WARN_INPUT_NO_SUPPORTED_EFFECTS"));
                }
            } else {
                D_PRINT_FN(Locale::get("WARN_INPUT_NO_FF_EFFECTS"));
            }
        }
    }
        
    ~JoystickInterface()
    {
        for(size_t nJoyInd = 0; nJoyInd < _vecJoys.size(); ++nJoyInd)
            _pInputInterface->destroyInputObject( _vecJoys[nJoyInd] );
    }
        
    inline size_t getNumberOfJoysticks() const { return _vecJoys.size(); }
    inline bool   wasFFDetected()        const { return _bFFFound; }
        
    enum EWhichJoystick { 
        ePrevious=-1,
        eNext=+1
    };
        
    void selectJoystick(EWhichJoystick eWhich) {
        // Note: Reset the master gain to half the maximum and autocenter mode to Off,
        // when really selecting a new joystick.
        if (_nCurrJoyInd < 0) {
            _nCurrJoyInd = 0;
            _dMasterGain = 0.5; // Half the maximum.
            changeMasterGain(0.0);
        } else {
            _nCurrJoyInd += eWhich;
            if (_nCurrJoyInd < -1 || _nCurrJoyInd >= (U8)_vecJoys.size())
                _nCurrJoyInd = -1;
            if (_vecJoys.size() > 1 && _nCurrJoyInd >= 0) {
                _dMasterGain = 0.5; // Half the maximum.
                changeMasterGain(0.0);
            }
        }
    }

    inline OIS::ForceFeedback* getCurrentFFDevice() {  return (_nCurrJoyInd >= 0) ? _vecFFDev[_nCurrJoyInd] : 0; }

    void changeMasterGain(F32 dDeltaPercent)  {
        if (_nCurrJoyInd < 0) 
            return;

        _dMasterGain += dDeltaPercent / 100;
                
        if (_dMasterGain > 1.0)
            _dMasterGain = 1.0;
        else if (_dMasterGain < 0.0)
            _dMasterGain = 0.0;

        _vecFFDev[_nCurrJoyInd]->setMasterGain(_dMasterGain);
    }

    enum EAutoCenterHow { 
        eOff,
        eOn,
        eToggle
    };

    void changeAutoCenter(EAutoCenterHow eHow = eToggle) {
        if (_nCurrJoyInd < 0)
            return;
      
        if (eHow == eToggle)
            _bAutoCenter = !_bAutoCenter;
        else
            _bAutoCenter = (eHow == eOn ? true : false);
            
        _vecFFDev[_nCurrJoyInd]->setAutoCenterMode(_bAutoCenter);
    }

    void captureEvents() {
        // This fires off buffered events for each joystick we have
        for(size_t nJoyInd = 0; nJoyInd < _vecJoys.size(); ++nJoyInd)
            if( _vecJoys[nJoyInd] )
                _vecJoys[nJoyInd]->capture();
    }
};

    }; //namespace Input
}; //namespace Divide
#endif
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

#ifndef _INPUT_MANAGER_H_
#define _INPUT_MANAGER_H_

#include "JoystickInterface.h"
#include "Core/Headers/Application.h"
#if defined OIS_WIN32_PLATFORM
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
class InputInterface;
class JoystickInterface;
class EffectManager;

void forceVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);
void periodVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);
//////////// Effect manager class //////////////////////////////////////////////////////////

class EffectManager{
  protected:

    // The joystick manager
    JoystickInterface* _pJoystickInterface;

    // vector to hold variable effects
    vectorImpl<VariableEffect*> _vecEffects;

    // Selected effect
    I8 _nCurrEffectInd;

    // Update frequency (Hz)
    U32 _nUpdateFreq;

    // Indices (in _vecEffects) of the variable effects that are playable by the selected joystick.
    vectorImpl<size_t> _vecPlayableEffectInd;

public:

    EffectManager(JoystickInterface* pJoystickInterface, U32 nUpdateFreq) : _pJoystickInterface(pJoystickInterface),
                                                                            _nUpdateFreq(nUpdateFreq),
                                                                            _nCurrEffectInd(-1)  
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

        _vecEffects.push_back(new VariableEffect("Constant force on 1 axis with 20s-period triangle oscillations "
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

            _vecEffects.push_back(new VariableEffect("Constant force on 1 axis with noticeable attack (app update period / 2)"
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
        _vecEffects.push_back(new VariableEffect("Periodic force on 1 axis with 40s-period triangle oscillations "
                                                    "of its period in [10, 400] ms, and constant amplitude",
                                                    pEffect, mapVars, periodVariableApplier));
    }

    ~EffectManager()
    {
        for (vectorImpl<VariableEffect*>::iterator iterEffs = _vecEffects.begin(); 
                                                   iterEffs != _vecEffects.end(); 
                                                   ++iterEffs)
           SAFE_DELETE(*iterEffs);
    }

    void updateActiveEffects() {
 
        for (vectorImpl<VariableEffect*>::iterator iterEffs = _vecEffects.begin();
                                                   iterEffs != _vecEffects.end();
                                                   ++iterEffs)
            if ((*iterEffs)->isActive()){
                (*iterEffs)->update();
                _pJoystickInterface->getCurrentFFDevice()->modify((*iterEffs)->getFFEffect());
            }
    }

    void checkPlayableEffects() {
        // Nothing to do if no joystick currently selected
        if (!_pJoystickInterface->getCurrentFFDevice()) return;

        // Get the list of indices of effects that the selected device can play
        _vecPlayableEffectInd.clear();
        for (size_t nEffInd = 0; nEffInd < _vecEffects.size(); ++nEffInd) {
            const OIS::Effect::EForce& eForce = _vecEffects[nEffInd]->getFFEffect()->force;
            const OIS::Effect::EType& eType = _vecEffects[nEffInd]->getFFEffect()->type;
            if (_pJoystickInterface->getCurrentFFDevice()->supportsEffect(eForce, eType))
                _vecPlayableEffectInd.push_back(nEffInd);
        }

        // Print details about playable effects
        if (_vecPlayableEffectInd.empty()) {
            D_ERROR_FN(Locale::get("INPUT_EFFECT_TEST_FAIL"));
        } else {
            PRINT_FN(Locale::get("INPUT_DEVICE_EFFECT_SUPPORT"));
            
            for (size_t nEffIndInd = 0; nEffIndInd < _vecPlayableEffectInd.size(); ++nEffIndInd)
                printEffect(_vecPlayableEffectInd[nEffIndInd]);
            
            PRINT_FN("");
        }
    }

    enum EWhichEffect { ePrevious=-1, eNone=0, eNext=+1 };

    void selectEffect(EWhichEffect eWhich) {
        // Nothing to do if no joystick currently selected
        if (!_pJoystickInterface->getCurrentFFDevice()) {
            D_PRINT_FN(Locale::get("INPUT_NO_JOYSTICK_SELECTED"));
            return;
        }

        // Nothing to do if joystick cannot play any effect
        if (_vecPlayableEffectInd.empty()) {
            D_PRINT_FN(Locale::get("INPUT_NO_PLAYABLE_EFFECTS"));
            return;
        }

        // If no effect selected, and next or previous requested, select the first one.
        if (eWhich != eNone && _nCurrEffectInd < 0)
            _nCurrEffectInd = 0;
        // Otherwise, remove the current one from the device and then select the requested one if any.
        else if (_nCurrEffectInd >= 0) { 
            _pJoystickInterface->getCurrentFFDevice()->remove(_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->getFFEffect());
            _vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->setActive(false);
            _nCurrEffectInd += eWhich;
            if (_nCurrEffectInd < -1 || _nCurrEffectInd >= (I16)_vecPlayableEffectInd.size())
                _nCurrEffectInd = -1;
        }

        // If no effect must be selected, reset the selection index
        if (eWhich == eNone) 
            _nCurrEffectInd = -1;
        // Otherwise, upload the new selected effect to the device if any.
        else if (_nCurrEffectInd >= 0) {
            _vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->setActive(true);
            _pJoystickInterface->getCurrentFFDevice()->upload(_vecEffects[_vecPlayableEffectInd[_nCurrEffectInd]]->getFFEffect());
        }
    }

    inline void printEffect(size_t nEffInd){
        PRINT_FN(Locale::get("INPUT_PRINT_EFFECT"),nEffInd,_vecEffects[nEffInd]->getDescription());
    }

    inline void printEffects(){
        for (size_t nEffInd = 0; nEffInd < _vecEffects.size(); nEffInd++){
          printEffect(nEffInd);
        }
    }
};

DEFINE_SINGLETON( InputInterface )

  protected:
    OIS::InputManager* _pInputInterface;
    EventHandler*      _pEventHdlr;
    OIS::Keyboard*     _pKeyboard;
    OIS::Mouse*        _pMouse;
    ///multiple joystick support
    vectorImpl<OIS::JoyStick* >	   _pJoysticks;

    JoystickInterface* _pJoystickInterface;
    EffectManager*     _pEffectMgr;

    bool               _bMustStop;
    bool               _bIsInitialized;

    I16 _nStatus;

    // App. heart beat frequency.
    static const U8 _nHartBeatFreq = 30; // Hz

    // Effects update frequency (Hz) : Needs to be quite lower than app. hart beat frequency,
    // if we want to be able to calmly study effect changes ...
    static const U8 _nEffectUpdateFreq = 5; // Hz

    InputInterface() : _pInputInterface(nullptr),
                       _pEventHdlr(nullptr),
                       _pKeyboard(nullptr),
                       _pJoystickInterface(nullptr),
                       _pMouse(nullptr),
                       _pEffectMgr(nullptr),
                       _bMustStop(false),
                       _bIsInitialized(false),
                       _nStatus(0)
    {
    }

    ~InputInterface()
    {
        terminate();
    }

public:
    ///Points to the position of said joystick in the vector
    enum Joysticks{
        JOY_1 = 0,
        JOY_2,
        JOY_3,
        JOY_4,
        JOY_5,
        JOY_6,
        JOY_7,
        JOY_8
    };

    U8 initialize(Kernel* const kernel, const std::string& windowTitle, size_t windowId = 0) {
        if(_bIsInitialized)	return NO_ERR;
        PRINT_FN(Locale::get("INPUT_CREATE_START"));
        OIS::ParamList pl;
#if defined OIS_WIN32_PLATFORM
      // Create OIS input manager
        pl.insert(std::make_pair(std::string("WINDOW"), Util::toString(windowId)));
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

        _pInputInterface = OIS::InputManager::createInputSystem(pl);
        assert(_pInputInterface);

        PRINT_FN(Locale::get("INPUT_CREATE_OK"),_pInputInterface->inputSystemName().c_str());

        // Create the event handler.
        _pEventHdlr = New EventHandler(this, kernel);
        assert(_pEventHdlr);

        try{
           // Create a simple keyboard
           _pKeyboard = (OIS::Keyboard*)_pInputInterface->createInputObject( OIS::OISKeyboard, true );
           _pKeyboard->setEventCallback( _pEventHdlr );
        }catch(OIS::Exception &ex){
            PRINT_FN(Locale::get("ERROR_INPUT_CREATE_KB"),ex.eText);
        }

        // Limit max joysticks to MAX_ALLOWED_JOYSTICKS
        U32 numJoysticks = std::min(_pInputInterface->getNumberOfDevices(OIS::OISJoyStick), MAX_ALLOWED_JOYSTICKS);

        if (numJoysticks > 0) {
            _pJoysticks.resize(numJoysticks);
            try{
                for(vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); it++){
                    (*it) = static_cast<OIS::JoyStick*>(_pInputInterface->createInputObject(OIS::OISJoyStick, true));
                    (*it)->setEventCallback( _pEventHdlr );
                }
            }catch(OIS::Exception &ex){
                PRINT_FN(Locale::get("ERROR_INPUT_CREATE_JOYSTICK"),ex.eText);
            }

            // Create the joystick manager.
            _pJoystickInterface = New JoystickInterface(_pInputInterface, _pEventHdlr);
            if( !_pJoystickInterface->wasFFDetected() )	{
                PRINT_FN(Locale::get("WARN_INPUT_NO_FORCE_FEEDBACK"));
                SAFE_DELETE(_pJoystickInterface);
            }else{
                // Create force feedback effect manager.
                _pEffectMgr = new EffectManager(_pJoystickInterface, _nEffectUpdateFreq);
                // Initialize the event handler.
                _pEventHdlr->initialize(_pJoystickInterface, _pEffectMgr);
            }
        }

        try{
            _pMouse = (OIS::Mouse*)_pInputInterface->createInputObject(OIS::OISMouse,true);
            _pMouse->setEventCallback( _pEventHdlr );
            const OIS::MouseState &ms = _pMouse->getMouseState(); //width and height are mutable
            ms.width = Application::getInstance().getResolution().width;
            ms.height = Application::getInstance().getResolution().height;
        }catch(OIS::Exception &ex){
            PRINT_FN(Locale::get("ERROR_INPUT_CREATE_MOUSE"),ex.eText);
        }

        _bIsInitialized = true;

        return _nStatus;
    }

    inline bool setMousePosition(const vec2<U16>& pos) const {
        return setMousePosition(pos.x, pos.y);
    }

    inline bool setMousePosition(U16 x, U16 y) const {
        Application::getInstance().setMousePosition(x, y);
        return true;
    }

    inline void updateResolution(U16 w,U16 h){
        const OIS::MouseState &ms = _pMouse->getMouseState(); //width and height are mutable
        ms.width = w;
        ms.height = h;
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

    U8 update(const U64 deltaTime) {

        const U8 nMaxEffectUpdateCnt = _nHartBeatFreq / _nEffectUpdateFreq;
        U8 nEffectUpdateCnt = 0;

        if (!_bIsInitialized) 
            return _nStatus;

        if(_bMustStop){
            terminate();
            return 0;
        }

        try {
            // This fires off buffered events for keyboards
            if(_pKeyboard != nullptr)  _pKeyboard->capture();
            if(_pMouse != nullptr)     _pMouse->capture();
            if(_pJoysticks.size() > 0){
                for(vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); it++){
                    (*it)->capture();
                }

                // This fires off buffered events for each joystick we have
                if(_pJoystickInterface != nullptr)  {
                    _pJoystickInterface->captureEvents();
                    // Update currently selected effects if time has come to.
                    if (!nEffectUpdateCnt) {
                        _pEffectMgr->updateActiveEffects();
                        nEffectUpdateCnt = nMaxEffectUpdateCnt;
                    } else
                        nEffectUpdateCnt--;
                }
            }
        } catch( ... ) { }
  
        return _nStatus;
    }

    inline void stop() {
        _bMustStop = true;
    }

    void terminate() {
        if (_pInputInterface) {
            _pInputInterface->destroyInputObject( _pKeyboard ); _pKeyboard = nullptr;
            _pInputInterface->destroyInputObject( _pMouse );    _pMouse = nullptr;

            if( _pJoysticks.size() > 0 ) {
                for(vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); it++){
                    _pInputInterface->destroyInputObject( *it );
                }
                _pJoysticks.clear();
                SAFE_DELETE(_pJoystickInterface);
            }
            
            OIS::InputManager::destroyInputSystem(_pInputInterface); _pInputInterface = nullptr;
        }

        SAFE_DELETE(_pEffectMgr);
        SAFE_DELETE(_pEventHdlr);

#if defined OIS_LINUX_PLATFORM
        // Be nice to X and clean up the x window
        XDestroyWindow(_pXDisp, _xWin);
        XCloseDisplay(_pXDisp);
#endif
    }

    inline JoystickInterface* getJoystickInterface(){ return _pJoystickInterface; }
    inline EffectManager* getEffectManager()        { return _pEffectMgr; }

END_SINGLETON

#endif
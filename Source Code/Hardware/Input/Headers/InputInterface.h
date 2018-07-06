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
    #include <windows.h>
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
    enum EWhichEffect { ePrevious = -1, eNone = 0, eNext = +1 };

    EffectManager(JoystickInterface* pJoystickInterface, U32 nUpdateFreq);
    ~EffectManager();

    void updateActiveEffects();
    void checkPlayableEffects();
    void selectEffect(EWhichEffect eWhich);

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

    U8 init(Kernel* const kernel, const std::string& windowTitle);

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

    U8 update(const U64 deltaTime);
    void terminate();

    inline void stop() { _bMustStop = true; }
    inline JoystickInterface* getJoystickInterface(){ return _pJoystickInterface; }
    inline EffectManager* getEffectManager()        { return _pEffectMgr; }

END_SINGLETON

#endif
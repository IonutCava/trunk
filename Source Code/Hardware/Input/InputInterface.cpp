#include "Headers/InputInterface.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {
    namespace Input {

U8 InputInterface::init(Kernel* const kernel, const stringImpl& windowTitle) {
    if (_bIsInitialized) {
        return NO_ERR;
    }

    PRINT_FN(Locale::get("INPUT_CREATE_START"));

    OIS::ParamList pl;
#   if defined OIS_WIN32_PLATFORM
		U32 hwnd = (U32)FindWindow(nullptr, ParamHandler::getInstance().getParam<stringImpl>("appTitle", "Divide").c_str());
        // Create OIS input manager
        pl.insert(std::make_pair(std::string("WINDOW"), Util::toString(hwnd)));
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND")));
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
        pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_FOREGROUND")));
        pl.insert(std::make_pair(std::string("w32_keyboard"), std::string("DISCL_NONEXCLUSIVE")));
#   elif defined OIS_LINUX_PLATFORM
        pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
        pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
        pl.insert(std::make_pair(std::string("x11_keyboard_grab"), std::string("false")));
        pl.insert(std::make_pair(std::string("XAutoRepeatOn"), std::string("true")));
#   endif

    _pInputInterface = OIS::InputManager::createInputSystem(pl);
    DIVIDE_ASSERT(_pInputInterface != nullptr, "InputInterface error: Could not create OIS Input Interface");

    PRINT_FN(Locale::get("INPUT_CREATE_OK"), _pInputInterface->inputSystemName().c_str());

    // Create the event handler.
    _pEventHdlr = New EventHandler(this, kernel);
    DIVIDE_ASSERT(_pEventHdlr != nullptr, "InputInterface error: EventHandler allocation failed!");

    try {
        // Create a simple keyboard
        _pKeyboard = static_cast<OIS::Keyboard*>(_pInputInterface->createInputObject(OIS::OISKeyboard, true));
        _pKeyboard->setEventCallback(_pEventHdlr);
    } catch (OIS::Exception &ex) {
        PRINT_FN(Locale::get("ERROR_INPUT_CREATE_KB"), ex.eText);
    }

    // Limit max joysticks to MAX_ALLOWED_JOYSTICKS
    U32 numJoysticks = std::min(_pInputInterface->getNumberOfDevices(OIS::OISJoyStick), MAX_ALLOWED_JOYSTICKS);

    if (numJoysticks > 0) {
        _pJoysticks.resize(numJoysticks);
        try {
            for (vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); ++it) {
                (*it) = static_cast<OIS::JoyStick*>(_pInputInterface->createInputObject(OIS::OISJoyStick, true));
                (*it)->setEventCallback(_pEventHdlr);
            }
        } catch (OIS::Exception &ex) {
            PRINT_FN(Locale::get("ERROR_INPUT_CREATE_JOYSTICK"), ex.eText);
        }

        // Create the joystick manager.
        _pJoystickInterface = New JoystickInterface(_pInputInterface, _pEventHdlr);
        if (!_pJoystickInterface->wasFFDetected())	{
            PRINT_FN(Locale::get("WARN_INPUT_NO_FORCE_FEEDBACK"));
            SAFE_DELETE(_pJoystickInterface);
        } else{
            // Create force feedback effect manager.
            _pEffectMgr = New EffectManager(_pJoystickInterface, _nEffectUpdateFreq);
            // Initialize the event handler.
            _pEventHdlr->initialize(_pJoystickInterface, _pEffectMgr);
        }
    }

    try {
        _pMouse = static_cast<OIS::Mouse*>(_pInputInterface->createInputObject(OIS::OISMouse, true));
        _pMouse->setEventCallback(_pEventHdlr);
        const OIS::MouseState &ms = _pMouse->getMouseState(); //width and height are mutable
        ms.width = Application::getInstance().getResolution().width;
        ms.height = Application::getInstance().getResolution().height;
    } catch (OIS::Exception &ex) {
        PRINT_FN(Locale::get("ERROR_INPUT_CREATE_MOUSE"), ex.eText);
    }

    _bIsInitialized = true;

    return _nStatus;
}

void InputInterface::updateResolution(U16 w,U16 h) {
    if (!_bIsInitialized) {
        return;
    }
    // width and height are mutable
    const OIS::MouseState &ms = _pMouse->getMouseState(); 
    ms.width = w;
    ms.height = h;
}

U8 InputInterface::update(const U64 deltaTime) {

    const U8 nMaxEffectUpdateCnt = _nHartBeatFreq / _nEffectUpdateFreq;
    U8 nEffectUpdateCnt = 0;

    if (!_bIsInitialized) {
        return _nStatus;
    }
    if (_bMustStop) {
        terminate();
        return 0;
    }

    // This fires off buffered events for keyboards
    if (_pKeyboard != nullptr) {
        _pKeyboard->capture();
    }
    if (_pMouse != nullptr) {
        _pMouse->capture();
    }
    if (_pJoysticks.size() > 0) {
        for (vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); ++it) {
            (*it)->capture();
        }

        // This fires off buffered events for each joystick we have
        if (_pJoystickInterface != nullptr)  {
            _pJoystickInterface->captureEvents();
            // Update currently selected effects if time has come to.
            if (!nEffectUpdateCnt) {
                _pEffectMgr->updateActiveEffects();
                nEffectUpdateCnt = nMaxEffectUpdateCnt;
            } else {
                nEffectUpdateCnt--;
            }
        }
    }

    return _nStatus;
}

void InputInterface::terminate() {
    if (_pInputInterface) {
        _pInputInterface->destroyInputObject(_pKeyboard);
        _pKeyboard = nullptr;

        _pInputInterface->destroyInputObject(_pMouse);    
        _pMouse = nullptr;

        if (_pJoysticks.size() > 0) {
            for (vectorImpl<OIS::JoyStick* >::iterator it = _pJoysticks.begin(); it != _pJoysticks.end(); ++it) {
                _pInputInterface->destroyInputObject(*it);
            }
            _pJoysticks.clear();
            SAFE_DELETE(_pJoystickInterface);
        }

        OIS::InputManager::destroyInputSystem(_pInputInterface);
        _pInputInterface = nullptr;
    }

    SAFE_DELETE(_pEffectMgr);
    SAFE_DELETE(_pEventHdlr);

#if defined OIS_LINUX_PLATFORM
    // Be nice to X and clean up the x window
    XDestroyWindow(_pXDisp, _xWin);
    XCloseDisplay(_pXDisp);
#endif
}

    }; //namespace Input
}; //namespace Divide
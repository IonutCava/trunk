#include "Headers/InputInterface.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {
namespace Input {

ErrorCode InputInterface::init(Kernel& kernel) {
    if (_bIsInitialized) {
        return ErrorCode::NO_ERR;
    }

    Console::printfn(Locale::get(_ID("INPUT_CREATE_START")));

    OIS::ParamList pl;

    std::stringstream ss;
    ss << (size_t)(Application::getInstance().getSysInfo()._windowHandle);
    // Create OIS input manager
    pl.insert(std::make_pair("WINDOW", ss.str()));
    pl.insert(std::make_pair("GLXWINDOW", ss.str()));
    pl.insert(std::make_pair("w32_mouse", "DISCL_FOREGROUND"));
    pl.insert(std::make_pair("w32_mouse", "DISCL_NONEXCLUSIVE"));
    pl.insert(std::make_pair("w32_keyboard", "DISCL_FOREGROUND"));
    pl.insert(std::make_pair("w32_keyboard", "DISCL_NONEXCLUSIVE"));
    pl.insert(std::make_pair("x11_mouse_grab", "false"));
    pl.insert(std::make_pair("x11_mouse_hide", "false"));
    pl.insert(std::make_pair("x11_keyboard_grab", "false"));
    pl.insert(std::make_pair("XAutoRepeatOn", "true"));

    _pInputInterface = OIS::InputManager::createInputSystem(pl);
    DIVIDE_ASSERT(_pInputInterface != nullptr,
                  "InputInterface error: Could not create OIS Input Interface");

    Console::printfn(Locale::get(_ID("INPUT_CREATE_OK")),
                     _pInputInterface->inputSystemName().c_str());

    // Create the event handler.
    _pEventHdlr = MemoryManager_NEW EventHandler(this, kernel);
    DIVIDE_ASSERT(_pEventHdlr != nullptr,
                  "InputInterface error: EventHandler allocation failed!");

    try {
        // Create a simple keyboard
        _pKeyboard = static_cast<OIS::Keyboard*>(
            _pInputInterface->createInputObject(OIS::OISKeyboard, true));
        _pKeyboard->setEventCallback(_pEventHdlr);
    } catch (OIS::Exception& ex) {
        Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_KB")), ex.eText);
    }

    // Limit max joysticks to MAX_ALLOWED_JOYSTICKS
    I32 numJoysticks =
        std::min(_pInputInterface->getNumberOfDevices(OIS::OISJoyStick),
                 to_int(Joystick::COUNT));

    if (numJoysticks > 0) {
        _pJoysticks.resize(numJoysticks);
        try {
            for (vectorImpl<OIS::JoyStick*>::iterator it =
                     std::begin(_pJoysticks);
                 it != std::end(_pJoysticks); ++it) {
                (*it) = static_cast<OIS::JoyStick*>(
                    _pInputInterface->createInputObject(OIS::OISJoyStick,
                                                        true));
                (*it)->setEventCallback(_pEventHdlr);
            }
        } catch (OIS::Exception& ex) {
            Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_JOYSTICK")), ex.eText);
        }

        // Create the joystick manager.
        _pJoystickInterface =
            MemoryManager_NEW JoystickInterface(_pInputInterface, _pEventHdlr);

        vectorAlg::vecSize joystickCount = _pJoysticks.size();
        for (vectorAlg::vecSize i = 0; i < joystickCount; ++i) {
            I32 max = _pJoysticks[i]->MAX_AXIS - 4000;
            _pJoystickInterface->setJoystickData(static_cast<Joystick>(i), JoystickData(max / 10, max));
        }
        if (!_pJoystickInterface->wasFFDetected()) {
            Console::printfn(Locale::get(_ID("WARN_INPUT_NO_FORCE_FEEDBACK")));
        } else {
            // Create force feedback effect manager.
            _pEffectMgr.reset(
                new EffectManager(_pJoystickInterface, _nEffectUpdateFreq));
            // Initialize the event handler.
            _pEventHdlr->initialize(_pJoystickInterface, _pEffectMgr.get());
        }
    }

    try {
        _pMouse = static_cast<OIS::Mouse*>(
            _pInputInterface->createInputObject(OIS::OISMouse, true));
        _pMouse->setEventCallback(_pEventHdlr);
        const OIS::MouseState& ms = _pMouse->getMouseState();  // width and height are mutable
        ms.width = Application::getInstance().getWindowManager().getResolution().width;
        ms.height = Application::getInstance().getWindowManager().getResolution().height;
    } catch (OIS::Exception& ex) {
        Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_MOUSE")), ex.eText);
    }

    _bIsInitialized = true;

    return ErrorCode::NO_ERR;
}

U8 InputInterface::update(const U64 deltaTime) {
    const U8 nMaxEffectUpdateCnt = _nHartBeatFreq / _nEffectUpdateFreq;
    U8 nEffectUpdateCnt = 0;

    if (!_bIsInitialized) {
        return 0;
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
        for (vectorImpl<OIS::JoyStick*>::iterator it = std::begin(_pJoysticks);
             it != std::end(_pJoysticks); ++it) {
            (*it)->capture();
        }

        // This fires off buffered events for each joystick we have
        if (_pEffectMgr != nullptr) {
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

    return 1;
}

void InputInterface::terminate() {
    if (_pInputInterface) {
        _pInputInterface->destroyInputObject(_pKeyboard);
        _pKeyboard = nullptr;

        _pInputInterface->destroyInputObject(_pMouse);
        _pMouse = nullptr;

        if (_pJoysticks.size() > 0) {
            for (vectorImpl<OIS::JoyStick*>::iterator it =
                     std::begin(_pJoysticks);
                 it != std::end(_pJoysticks); ++it) {
                _pInputInterface->destroyInputObject(*it);
            }
            _pJoysticks.clear();
            MemoryManager::DELETE(_pJoystickInterface);
        }

        OIS::InputManager::destroyInputSystem(_pInputInterface);
        _pInputInterface = nullptr;
    }

    MemoryManager::DELETE(_pEventHdlr);
}

};  // namespace Input
};  // namespace Divide

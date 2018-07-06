#include "stdafx.h"

#include "Headers/JoystickInterface.h"

#include "Utility/Headers/Localization.h"
#include "Core/Headers/Console.h"

namespace Divide {
namespace Input {

JoystickInterface::JoystickInterface(OIS::InputManager* pInputInterface, EventHandler* pEventHdlr)
    : _pInputInterface(pInputInterface),
    _nCurrJoyInd(-1),
    _dMasterGain(0.5),
    _bAutoCenter(true)
{
    _bFFFound = false;
    for (U8 nJoyInd = 0;
        nJoyInd < _pInputInterface->getNumberOfDevices(OIS::OISJoyStick);
        ++nJoyInd) {
        OIS::JoyStick* pJoy = nullptr;
        OIS::ForceFeedback* pFFDev = nullptr;

        try {
            pJoy = static_cast<OIS::JoyStick*>(
                _pInputInterface->createInputObject(OIS::OISJoyStick,
                    true));
            if (pJoy) {
                Console::printfn(Locale::get(_ID("INPUT_CREATE_BUFF_JOY")),
                    nJoyInd, pJoy->vendor().empty()
                    ? "unknown vendor"
                    : pJoy->vendor().c_str(),
                    pJoy->getID());
                // Check for FF, and if so, keep the joy and dump FF info
                pFFDev = (OIS::ForceFeedback*)pJoy->queryInterface(
                    OIS::Interface::ForceFeedback);
            }
        }
        catch (OIS::Exception& ex) {
            Console::printfn(Locale::get(_ID("ERROR_INPUT_CREATE_JOYSTICK")),
                ex.eText);
        }

        if (pFFDev) {
            _bFFFound = true;
            // Keep the joy to play with it.
            pJoy->setEventCallback(pEventHdlr);
            _vecJoys.push_back(pJoy);
            // Keep also the associated FF device
            _vecFFDev.push_back(pFFDev);
            // Dump FF supported effects and other info.
            Console::printfn(Locale::get(_ID("INPUT_JOY_NUM_FF_AXES")),
                pFFDev->getFFAxesNumber());
            const OIS::ForceFeedback::SupportedEffectList& lstFFEffects =
                pFFDev->getSupportedEffects();

            if (lstFFEffects.size() > 0) {
                Console::printf(Locale::get(_ID("INPUT_JOY_SUPPORTED_EFFECTS")));
                OIS::ForceFeedback::SupportedEffectList::const_iterator
                    itFFEff;
                for (itFFEff = std::begin(lstFFEffects);
                    itFFEff != std::end(lstFFEffects); ++itFFEff)
                    Console::printf(" %s\n", OIS::Effect::getEffectTypeName(
                        itFFEff->second));
            }
            else {
                Console::d_printfn(
                    Locale::get(_ID("WARN_INPUT_NO_SUPPORTED_EFFECTS")));
            }
        }
        else {
            Console::d_printfn(Locale::get(_ID("WARN_INPUT_NO_FF_EFFECTS")));
        }
    }
}

JoystickInterface::~JoystickInterface()
{
    for (vectorAlg::vecSize nJoyInd = 0; nJoyInd < _vecJoys.size();
        ++nJoyInd) {
        _pInputInterface->destroyInputObject(_vecJoys[nJoyInd]);
    }
}

}; //namespace Input
}; //namespace Divide
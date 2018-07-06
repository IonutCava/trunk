#include "Headers/EffectManager.h"

namespace Divide {
    namespace Input {

EffectManager::EffectManager(JoystickInterface* pJoystickInterface, U32 nUpdateFreq) : _pJoystickInterface(pJoystickInterface),
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
        4 * 10000 / _nUpdateFreq / 20.0, // dF for a 20s-period triangle
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
    pConstForce->envelope.attackLength = (U32)(1000000.0 / _nUpdateFreq / 2);
    pConstForce->envelope.attackLevel = (U16)(pConstForce->level*0.1);
    pConstForce->envelope.fadeLength = 0; // Never reached, actually.
    pConstForce->envelope.fadeLevel = (U16)pConstForce->level; // Idem

    mapVars.clear();
    mapVars["Force"] =
        new TriangleVariable(0.0, // F0
        4 * 10000 / _nUpdateFreq / 20.0, // dF for a 20s-period triangle
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
        new TriangleVariable(1 * 1000.0, // P0
        4 * (400 - 10)*1000.0 / _nUpdateFreq / 40.0, // dP for a 40s-period triangle
        10 * 1000.0, // Pmin
        400 * 1000.0); // Pmax
    _vecEffects.push_back(new VariableEffect("Periodic force on 1 axis with 40s-period triangle oscillations "
        "of its period in [10, 400] ms, and constant amplitude",
        pEffect, mapVars, periodVariableApplier));
}

EffectManager::~EffectManager()
{
    for (vectorImpl<VariableEffect*>::iterator iterEffs = _vecEffects.begin();
        iterEffs != _vecEffects.end();
        ++iterEffs)
        SAFE_DELETE(*iterEffs);
}

void EffectManager::updateActiveEffects() {

    for (vectorImpl<VariableEffect*>::iterator iterEffs = _vecEffects.begin();
        iterEffs != _vecEffects.end();
        ++iterEffs)
    if ((*iterEffs)->isActive()){
        (*iterEffs)->update();
        _pJoystickInterface->getCurrentFFDevice()->modify((*iterEffs)->getFFEffect());
    }
}

void EffectManager::checkPlayableEffects() {
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
    }
    else {
        PRINT_FN(Locale::get("INPUT_DEVICE_EFFECT_SUPPORT"));

        for (size_t nEffIndInd = 0; nEffIndInd < _vecPlayableEffectInd.size(); ++nEffIndInd)
            printEffect(_vecPlayableEffectInd[nEffIndInd]);

        PRINT_FN("");
    }
}

void EffectManager::selectEffect(EWhichEffect eWhich) {
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

    }; //namespace Input
}; //namespace Divide
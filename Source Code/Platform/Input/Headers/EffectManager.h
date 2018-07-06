/*
   Copyright (c) 2017 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _EFFECT_MANAGER_H_
#define _EFFECT_MANAGER_H_

#include "JoystickInterface.h"

namespace Divide {
namespace Input {

void forceVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);
void periodVariableApplier(MapVariables& mapVars, OIS::Effect* pEffect);

//////////// Effect manager class
/////////////////////////////////////////////////////////////
class EffectManager {
   protected:
    // The joystick manager
    JoystickInterface* _pJoystickInterface;

    // vector to hold variable effects
    vectorImpl<VariableEffect*> _vecEffects;

    // Selected effect
    I32 _nCurrEffectInd;

    // Update frequency (Hz)
    U32 _nUpdateFreq;

    // Indices (in _vecEffects) of the variable effects that are playable by the
    // selected joystick.
    vectorImpl<vectorAlg::vecSize> _vecPlayableEffectInd;

   public:
    enum class EWhichEffect : I32 {
        ePrevious = -1,
        eNone = 0,
        eNext = +1
    };

    EffectManager(JoystickInterface* pJoystickInterface, U32 nUpdateFreq);
    ~EffectManager();

    void updateActiveEffects();
    void checkPlayableEffects();
    void selectEffect(EWhichEffect eWhich);

    void printEffect(vectorAlg::vecSize nEffInd);

    void printEffects();
};
};  // namespace Input
};  // namespace Divide
#endif
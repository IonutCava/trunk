/*
   Copyright (c) 2015 DIVIDE-Studio
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
#ifndef _WAR_SCENE_AESOP_ACTION_INTERFACE_H_
#define _WAR_SCENE_AESOP_ACTION_INTERFACE_H_

#include "AI/ActionInterface/Headers/GOAPInterface.h"

namespace Divide {
namespace AI {

enum class ActionType : U32 {
    APPROACH_ENEMY_FLAG = 0,
    CAPTURE_ENEMY_FLAG = 1,
    SCORE_FLAG = 2,
    RETURN_TO_BASE = 3,
    IDLE = 4,
    ATTACK_ENEMY = 5,
    RECOVER_FLAG = 6,
    COUNT
};

// Some useful predicates
enum class Fact : U32 {
    NEAR_ENEMY_FLAG = 0,
    AT_HOME_BASE = 1,
    HAS_ENEMY_FLAG = 2,
    ENEMY_HAS_FLAG = 3,
    IDLING = 4,
    ENEMY_DEAD = 5,
    COUNT
};

inline const char* WarSceneFactName(GOAPFact fact) {
    switch (static_cast<Fact>(fact)) {
        case Fact::NEAR_ENEMY_FLAG:
            return "NEAR ENEMY FLAG";
        case Fact::AT_HOME_BASE:
            return "AT HOME BASE";
        case Fact::HAS_ENEMY_FLAG:
            return "HAS ENEMY FLAG";
        case Fact::ENEMY_HAS_FLAG:
            return "ENEMY HAS FLAG";
        case Fact::IDLING:
            return "IDLING";
        case Fact::ENEMY_DEAD:
            return "ENEMY DEAD";
        default:
            return "ERROR!";
    };
    return GOAPFactName(fact);
};

class WarSceneAISceneImpl;
class WarSceneAction : public GOAPAction {
   public:
    WarSceneAction(ActionType type, const stringImpl& name, F32 cost = 1.0f);
    virtual ~WarSceneAction();

    inline ActionType actionType() const { return _type; }

    bool preAction(WarSceneAISceneImpl& parentScene) const;
    bool postAction(WarSceneAISceneImpl& parentScene) const;
    virtual bool checkImplDependentCondition() const { return true; }

   protected:
    ActionType _type;
};

};  // namespace AI
};  // namespace Divide

#endif

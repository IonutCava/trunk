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

// Some useful predicates
enum class Fact : U32 {
    AtEnemyFlagLoc = 0,
    AtHomeFlagLoc = 1,
    HasEnemyFlag = 2,
    Idling = 3,
    COUNT
};

inline const char* WarSceneFactName(GOAPFact fact) {
    switch (static_cast<Fact>(fact)) {
        case Fact::AtEnemyFlagLoc:
            return "At enemy flag location";
        case Fact::AtHomeFlagLoc:
            return "At home location";
        case Fact::HasEnemyFlag:
            return "Has enemy flag";
        case Fact::Idling:
            return "Idling";
    };
    return GOAPFactName(fact);
};

enum class ActionType : U32 {
    ACTION_APPROACH_FLAG = 0,
    ACTION_CAPTURE_FLAG = 1,
    ACTION_SCORE_FLAG = 2,
    ACTION_RETURN_FLAG_TO_BASE = 3,
    ACTION_IDLE = 4,
    COUNT
};

namespace Attorney {
    class WarSceneActionWarAIScene;
};

class WarSceneAISceneImpl;
class WarSceneAction : public GOAPAction {
    friend class Attorney::WarSceneActionWarAIScene;

   public:
    inline ActionType actionType() const { return _type; }

    bool preAction() const;
    bool postAction() const;
    virtual bool checkImplDependentCondition() const { return true; }

   protected:
    WarSceneAction(ActionType type, const stringImpl& name, F32 cost = 1.0f);
    virtual ~WarSceneAction();

   protected:
    WarSceneAISceneImpl* _parentScene;
    ActionType _type;
};

namespace Attorney {
class WarSceneActionWarAIScene {
   private:
    static void setParentAIScene(WarSceneAction& action,
                                 Divide::AI::WarSceneAISceneImpl* const scene) {
        action._parentScene = scene;
    }

    friend class Divide::AI::WarSceneAISceneImpl;
};
};  // namespace Attorney

class Idle : public WarSceneAction {
  public:
    Idle(const stringImpl& name, F32 cost = 1.0f);
    Idle(WarSceneAction const& other) : WarSceneAction(other) {}
};

class ApproachFlag : public WarSceneAction {
   public:
    ApproachFlag(const stringImpl& name, F32 cost = 1.0f);
    ApproachFlag(WarSceneAction const& other) : WarSceneAction(other) {}
};

class CaptureFlag : public WarSceneAction {
   public:
    CaptureFlag(const stringImpl& name, F32 cost = 1.0f);
    CaptureFlag(WarSceneAction const& other) : WarSceneAction(other) {}
};

class ReturnFlagHome : public WarSceneAction {
   public:
    ReturnFlagHome(const stringImpl& name, F32 cost = 1.0f);
    ReturnFlagHome(WarSceneAction const& other) : WarSceneAction(other) {}
};

class ScoreFlag : public WarSceneAction {
   public:
    ScoreFlag(const stringImpl& name, F32 cost = 1.0f);
    ScoreFlag(WarSceneAction const& other) : WarSceneAction(other) {}
};

};  // namespace AI
};  // namespace Divide

#endif

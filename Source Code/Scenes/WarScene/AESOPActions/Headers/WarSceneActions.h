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
#ifndef _WAR_SCENE_AESOP_ACTION_INTERFACE_H_
#define _WAR_SCENE_AESOP_ACTION_INTERFACE_H_

#include "AI/ActionInterface/Headers/GOAPInterface.h"

namespace AI {
    // Some useful predicates
    enum Fact {
        EnemyVisible = 0,
        EnemyInAttackRange = 1,
        EnemyDead = 2,
        WaitingIdle = 3,
        AtTargetNode = 4
    };

    inline const char* WarSceneFactName(GOAPFact fact) {
        switch(static_cast<Fact>(fact)){
            case EnemyVisible : return "Enemy Visible";
            case EnemyInAttackRange : return "Enemy In Attack Range";
            case EnemyDead : return "Enemy Dead";
            case WaitingIdle : return "Waiting Idle";
            case AtTargetNode : return "At Target Node";
        };
        return GOAPFactName(fact);
    };

    enum ActionType {
        ACTION_WAIT = 0,
        ACTION_SCOUT = 1,
        ACTION_APPROACH = 2,
        ACTION_TARGET = 3,
        ACTION_ATTACK = 4,
        ACTION_RETREAT = 5,
        ACTION_KILL = 6,
        ACTION_DEFAULT = 7
    };
    
    enum Order {
        ORDER_FIND_ENEMY = 0,
        ORDER_KILL_ENEMY = 1,
        ORDER_WAIT = 2
    };

    class WarSceneGoal : public GOAPGoal {
        public:
            WarSceneGoal(const std::string& name);
            WarSceneGoal(GOAPGoal const & other);
            virtual ~WarSceneGoal();
            void update(const U64 deltaTime);
            void evaluateRelevancy(AISceneImpl* const AIScene);
    
        private:
            bool _relevancyUpdateQueued;
            AISceneImpl* _queuedScene;
    };

    class WarSceneAction : public GOAPAction {
        public:
            WarSceneAction(ActionType type, const std::string& name, F32 cost = 1.0f);
            virtual ~WarSceneAction();

            inline ActionType actionType() const { return _type; }

            virtual bool preAction() const;
            virtual bool postAction() const;
            virtual bool checkImplDependentCondition() const {
                return true;
            }

        protected:
            ActionType _type;
    };

    class WaitAction : public WarSceneAction {
        public:
            WaitAction(std::string name, F32 cost = 1.0f);
            WaitAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class ScoutAction : public WarSceneAction {
        public:
            ScoutAction(std::string name, F32 cost = 1.0f);
            ScoutAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class ApproachAction : public WarSceneAction {
        public:
            ApproachAction(std::string name, F32 cost = 1.0f);
            ApproachAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class TargetAction : public WarSceneAction {
        public:
            TargetAction(std::string name, F32 cost = 1.0f);
            TargetAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class AttackAction : public WarSceneAction {
        public:
            AttackAction(std::string name, F32 cost = 1.0f);
            AttackAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class RetreatAction : public WarSceneAction {
        public:
            RetreatAction(std::string name, F32 cost = 1.0f);
            RetreatAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
    class KillAction : public WarSceneAction {
        public:
            KillAction(std::string name, F32 cost = 1.0f);
            KillAction(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
            bool preAction() const;
            bool postAction() const;
    };
};
#endif
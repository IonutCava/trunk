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

namespace Divide {
    namespace AI {

    // Some useful predicates
    enum Fact {
        EnemyVisible = 0,
        EnemyInAttackRange = 1,
        EnemyDead = 2,
        WaitingIdle = 3,
        AtTargetNode = 4,
        HasTargetNode = 5
    };

    inline const char* WarSceneFactName(GOAPFact fact) {
        switch(static_cast<Fact>(fact)){
            case EnemyVisible : return "Enemy Visible";
            case EnemyInAttackRange : return "Enemy In Attack Range";
            case EnemyDead : return "Enemy Dead";
            case WaitingIdle : return "Waiting Idle";
            case AtTargetNode : return "At Target Node";
            case HasTargetNode : return "Has Target Node";
        };
        return GOAPFactName(fact);
    };

    enum ActionType {
        ACTION_APPROACH_FLAG = 0,
        ACTION_CAPTURE_FLAG = 1,
        ACTION_RETURN_FLAG = 2,
        ACTION_PROTECT_FLAG_CARRIER = 3,
        ACTION_RECOVER_FLAG = 4
    };
    
    class WarSceneAction : public GOAPAction {
        public:
            inline ActionType actionType() const { return _type; }

            bool preAction() const;
            bool postAction() const;
            virtual bool checkImplDependentCondition() const {
                return true;
            }

        protected:
            WarSceneAction(ActionType type, const stringImpl& name, F32 cost = 1.0f);
            virtual ~WarSceneAction();

        protected:
            friend class WarSceneAISceneImpl;
            inline void  setParentAIScene(WarSceneAISceneImpl* const scene) {
                _parentScene = scene;
            }

        protected:
            WarSceneAISceneImpl* _parentScene;
            ActionType _type;
    };

    class ApproachFlag : public WarSceneAction {
        public:
            ApproachFlag(stringImpl name, F32 cost = 1.0f);
            ApproachFlag(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
    };
    class CaptureFlag : public WarSceneAction {
        public:
            CaptureFlag(stringImpl name, F32 cost = 1.0f);
            CaptureFlag(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
    };
    class ReturnFlag : public WarSceneAction {
        public:
            ReturnFlag(stringImpl name, F32 cost = 1.0f);
            ReturnFlag(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
    };
    class ProtectFlagCarrier : public WarSceneAction {
        public:
            ProtectFlagCarrier(stringImpl name, F32 cost = 1.0f);
            ProtectFlagCarrier(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
        
    };
    class RecoverFlag : public WarSceneAction {
        public:
            RecoverFlag(stringImpl name, F32 cost = 1.0f);
            RecoverFlag(WarSceneAction const & other) : WarSceneAction(other)
            {
            }
    };
    }; //namespace AI
}; //namespace Divide

#endif
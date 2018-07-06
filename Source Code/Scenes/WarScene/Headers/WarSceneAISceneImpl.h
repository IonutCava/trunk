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

#ifndef _WAR_SCENE_AI_ACTION_LIST_H_
#define _WAR_SCENE_AI_ACTION_LIST_H_

#include "AI/ActionInterface/Headers/AITeam.h"
#include "AI/ActionInterface/Headers/AISceneImpl.h"
#include "Scenes/WarScene/AESOPActions/Headers/WarSceneActions.h"

namespace Divide {

namespace AI {
enum AIMsg { HAVE_FLAG = 0, ENEMY_HAS_FLAG = 1 };

enum FactType {
    FACT_TYPE_POSITION = 0,
    FACT_TYPE_COUNTER_SMALL = 1,
    FACT_TYPE_COUNTER_MEDIUM = 2,
    FACT_TYPE_COUNTER_LARGE = 3,
    FACT_TYPE_TOGGLE_STATE = 4,
    FACT_TYPE_AI_NODE = 5,
    FACT_TYPE_SGN_NODE = 6,
    FactType_PLACEHOLDER = 7
};

template <typename T, FactType F>
class WorkingMemoryFact {
   public:
    WorkingMemoryFact() {
        _value = 0;
        _type = F;
        _belief = 0.0f;
    }

    inline void value(const T& val) {
        _value = val;
        belief(1.0f);
    }
    inline void belief(F32 belief) { _belief = belief; }

    inline const T& value() const { return _value; }
    inline FactType type() const { return _type; }
    inline F32 belief() const { return _belief; }

   protected:
    T _value;
    F32 _belief;
    FactType _type;
};

typedef WorkingMemoryFact<AIEntity*, FACT_TYPE_AI_NODE> AINodeFact;
typedef WorkingMemoryFact<SceneGraphNode*, FACT_TYPE_SGN_NODE> SGNNodeFact;
typedef WorkingMemoryFact<vec3<F32>, FACT_TYPE_POSITION> PositionFact;
typedef WorkingMemoryFact<U8, FACT_TYPE_COUNTER_SMALL> SmallCounterFact;
typedef WorkingMemoryFact<U16, FACT_TYPE_COUNTER_MEDIUM> MediumCounterFact;
typedef WorkingMemoryFact<U32, FACT_TYPE_COUNTER_LARGE> LargeCounterFact;
typedef WorkingMemoryFact<bool, FACT_TYPE_TOGGLE_STATE> ToggleStateFact;

class WorkingMemory {
   public:
    WorkingMemory() {
        _hasEnemyFlag.value(false);
        _enemyHasFlag.value(false);
        _enemyFlagNear.value(false);
        _friendlyFlagNear.value(true);
        _teamMateHasFlag.value(false);
        _health.value(100);
        _flagCarrier.value(nullptr);
        _enemyFlagCarrier.value(nullptr);
        _currentTargetEntity.value(nullptr);
        _staticDataUpdated = false;
    }
    static SGNNodeFact _flags[2];
    static SmallCounterFact _teamCount[2];
    static SmallCounterFact _flagProtectors[2];
    static SmallCounterFact _flagRetrievers[2];
    static PositionFact _teamFlagPosition[2];
    AINodeFact _flagCarrier;
    AINodeFact _enemyFlagCarrier;
    SmallCounterFact _health;
    SGNNodeFact _currentTargetEntity;
    PositionFact _currentTargetPosition;
    ToggleStateFact _hasEnemyFlag;
    ToggleStateFact _enemyHasFlag;
    ToggleStateFact _teamMateHasFlag;
    ToggleStateFact _enemyFlagNear;
    ToggleStateFact _friendlyFlagNear;
    bool _staticDataUpdated;
};

class WarSceneOrder : public Order {
   public:
    enum WarOrder {
        ORDER_FIND_ENEMY_FLAG = 0,
        ORDER_CAPTURE_ENEMY_FLAG = 1,
        ORDER_RETURN_ENEMY_FLAG = 2,
        ORDER_PROTECT_FLAG_CARRIER = 3,
        ORDER_RETRIEVE_FLAG = 4,
        ORDER_PLACEHOLDER = 5
    };
    WarSceneOrder(WarOrder order) : Order(static_cast<U32>(order)) {}
    void evaluatePriority();
    void lock() { Order::lock(); }
    void unlock() { Order::unlock(); }
};

class WarSceneAISceneImpl : public AISceneImpl {
    friend class WarAISceneWarActionAttorney;

   public:
    WarSceneAISceneImpl();
    ~WarSceneAISceneImpl();

    void processData(const U64 deltaTime);
    void processInput(const U64 deltaTime);
    void update(const U64 deltaTime, NPC* unitRef = nullptr);
    void processMessage(AIEntity* sender, AIMsg msg,
                        const cdiggins::any& msg_content);
    void registerAction(GOAPAction* const action);

    static void registerFlags(SceneGraphNode* const flag1,
                              SceneGraphNode* const flag2) {
        WorkingMemory::_flags[0].value(flag1);
        WorkingMemory::_flags[1].value(flag2);
    }

   protected:
    bool preAction(ActionType type, const WarSceneAction* warAction);
    bool postAction(ActionType type, const WarSceneAction* warAction);
    bool checkCurrentActionComplete(const GOAPAction* planStep);

   private:
    void requestOrders();
    void updatePositions();
    bool performAction(const GOAPAction* planStep);
    bool performActionStep(GOAPAction::operationsIterator step);
    bool printActionStats(const GOAPAction* planStep) const;
    void printWorkingMemory() const;
    void initInternal();

   private:
    U16 _tickCount;
    U64 _deltaTime;
    U8 _orderRequestTryCount;
    U8 _visualSensorUpdateCounter;
    VisualSensor* _visualSensor;
    AudioSensor* _audioSensor;
    WorkingMemory _workingMemory;
    static vec3<F32> _initialFlagPositions[2];
};

class WarAISceneWarActionAttorney {
   private:
    static bool preAction(WarSceneAISceneImpl& aiScene, ActionType type,
                          const WarSceneAction* warAction) {
        return aiScene.preAction(type, warAction);
    }
    static bool postAction(WarSceneAISceneImpl& aiScene, ActionType type,
                           const WarSceneAction* warAction) {
        return aiScene.postAction(type, warAction);
    }

    friend class WarSceneAction;
};

};  // namespace AI
};  // namespace Divide

#endif
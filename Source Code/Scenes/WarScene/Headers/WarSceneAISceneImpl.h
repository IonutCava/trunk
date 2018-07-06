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

#ifndef _WAR_SCENE_AI_ACTION_LIST_H_
#define _WAR_SCENE_AI_ACTION_LIST_H_

#include "AI/ActionInterface/Headers/AISceneImpl.h"
#include "AESOPActions/Headers/WarSceneActions.h"

namespace AI {
enum AIMsg {
    REQUEST_DISTANCE_TO_TARGET = 0,
    RECEIVE_DISTANCE_TO_TARGET = 1,
    CHANGE_DESTINATION_POINT = 2
};

enum FactType {
    FACT_TYPE_POSITION       = 0,
    FACT_TYPE_COUNTER_SMALL  = 1,
    FACT_TYPE_COUNTER_MEDIUM = 2,
    FACT_TYPE_COUNTER_LARGE  = 3,
    FACT_TYPE_AI_NODE        = 4,
    FACT_TYPE_SGN_NODE       = 5,
    FactType_PLACEHOLDER     = 6
};

template<typename T, FactType F>
class WorkingMemoryFact {
public:

    WorkingMemoryFact()
    {
        _value = 0;
        _type = F;
        _belief = 0.0f;
    }

    inline void value(const T& val) { _value = val; }
    inline void belief(F32 belief)  { _belief = belief; }

    inline const T& value()  { return _value; }
    inline FactType type()   { return _type; }
    inline F32      belief() { return _belief; }

protected:
    T   _value;
    F32 _belief;
    FactType _type;
};

typedef WorkingMemoryFact<AIEntity*, FACT_TYPE_AI_NODE>          AINodeFact;
typedef WorkingMemoryFact<SceneGraphNode*, FACT_TYPE_SGN_NODE>   SGNNodeFact;
typedef WorkingMemoryFact<vec3<F32>, FACT_TYPE_POSITION>         PositionFact;
typedef WorkingMemoryFact<U8,  FACT_TYPE_COUNTER_SMALL>          SmallCounterFact;
typedef WorkingMemoryFact<U16, FACT_TYPE_COUNTER_MEDIUM>         MediumCounterFact;
typedef WorkingMemoryFact<U32, FACT_TYPE_COUNTER_LARGE>          LargeCounterFact;

class WorkingMemory {
public:
    WorkingMemory() 
    {
        _health.value(100);
        _currentTargetEntity.value(nullptr);  
        _staticDataUpdated = false;
    }
    static PositionFact     _team1FlagPosition;
    static PositionFact     _team2FlagPosition;
    static SmallCounterFact _team1Count;
    static SmallCounterFact _team2Count;
    static SGNNodeFact      _flags[2];
           SmallCounterFact _health;
           AINodeFact       _currentTargetEntity;
           PositionFact     _currentTargetPosition;

    bool _staticDataUpdated;
};

class WarSceneAISceneImpl : public AI::AISceneImpl {
public:
    WarSceneAISceneImpl();
    ~WarSceneAISceneImpl();

    void processData(const U64 deltaTime);
    void processInput(const U64 deltaTime);
    void update(const U64 deltaTime, NPC* unitRef = nullptr);
    void processMessage(AIEntity* sender, AIMsg msg,const cdiggins::any& msg_content);
    void registerAction(GOAPAction* const action);
    void registerGoal(const GOAPGoal& goal);

    static void registerFlags(SceneGraphNode* const flag1, SceneGraphNode* const flag2) {
        WorkingMemory::_flags[0].value(flag1);
        WorkingMemory::_flags[0].belief(1.0f);
        WorkingMemory::_flags[1].value(flag2);
        WorkingMemory::_flags[1].belief(1.0f);
    }

private:
    void updatePositions();
    // Creates a copy of the specified object and adds it to the action vector and the ActionSet

    void handlePlan(const GOAPPlan& plan);
    bool performAction(const GOAPAction* planStep);
    void receiveOrder(AI::Order order);
    void init();

private:
    U16       _tickCount;
    I32       _indexInMap;
    U64       _deltaTime;
    AIEntity* _currentEnemyTarget;
    GOAPGoal* _activeGoal;
    bool _newPlan;
    bool _newPlanSuccess;
    bool _orderReceived;
    U8   _visualSensorUpdateCounter;
    WorkingMemory _workingMemory;
};

}; //namespace AI
#endif
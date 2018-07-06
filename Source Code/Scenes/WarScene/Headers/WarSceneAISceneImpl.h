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
#include <fstream>

#ifndef PRINT_AI_TO_FILE
#define PRINT_AI_TO_FILE
#endif

namespace Divide {
class Unit;

namespace AI {

enum class UnitAttributes : U32 {
    HEALTH_POINTS = 0,
    DAMAGE = 1,
    ALIVE_FLAG = 2,
    COUNT
};

enum class AIMsg : U32 { 
    HAVE_FLAG = 0,
    RETURNED_FLAG = 1,
    ENEMY_HAS_FLAG = 2,
    HAVE_SCORED = 3,
    ATTACK = 4,
    HAVE_DIED = 5,
    COUNT
};

enum class FactType : U32 {
    POSITION = 0,
    COUNTER_SMALL = 1,
    COUNTER_MEDIUM = 2,
    COUNTER_LARGE = 3,
    TOGGLE_STATE = 4,
    AI_NODE = 5,
    SGN_NODE = 6,
    COUNT
};

template <typename T, FactType F>
class WorkingMemoryFact {
   public:
    WorkingMemoryFact()
    {
        _value = T();
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

typedef WorkingMemoryFact<AIEntity*, FactType::AI_NODE> AINodeFact;
typedef WorkingMemoryFact<std::weak_ptr<SceneGraphNode>, FactType::SGN_NODE> SGNNodeFact;
typedef WorkingMemoryFact<vec3<F32>, FactType::POSITION> PositionFact;
typedef WorkingMemoryFact<U8, FactType::COUNTER_SMALL> SmallCounterFact;
typedef WorkingMemoryFact<U16, FactType::COUNTER_MEDIUM> MediumCounterFact;
typedef WorkingMemoryFact<U32, FactType::COUNTER_LARGE> LargeCounterFact;
typedef WorkingMemoryFact<bool, FactType::TOGGLE_STATE> ToggleStateFact;

class GlobalWorkingMemory {
public:
    GlobalWorkingMemory()
    {
        _flags[0].value(std::weak_ptr<SceneGraphNode>());
        _flags[1].value(std::weak_ptr<SceneGraphNode>());
        _flagCarriers[0].value(nullptr);
        _flagCarriers[1].value(nullptr);
        _flagsAtBase[0].value(true);
        _flagsAtBase[1].value(true);
    }

    SGNNodeFact _flags[2];
    AINodeFact  _flagCarriers[2];
    SmallCounterFact _score[2];
    SmallCounterFact _teamAliveCount[2];
    SmallCounterFact _flagRetrieverCount[2];
    PositionFact _teamFlagPosition[2];
    ToggleStateFact _flagsAtBase[2];
};

class LocalWorkingMemory {
   public:
    LocalWorkingMemory()
    {
        _hasEnemyFlag.value(false);
        _enemyHasFlag.value(false);
       _isFlagRetriever.value(false);
       _currentTarget.value(std::weak_ptr<SceneGraphNode>());
    }

    SGNNodeFact _currentTarget;
    ToggleStateFact _hasEnemyFlag;
    ToggleStateFact _enemyHasFlag;
    ToggleStateFact _isFlagRetriever;
};

class WarSceneOrder : public Order {
   public:
    enum class WarOrder : U32 {
        IDLE = 0,
        CAPTURE_ENEMY_FLAG = 1,
        SCORE_ENEMY_FLAG = 2,
        KILL_ENEMY = 3,
        PROTECT_FLAG_CARRIER = 4,
        RECOVER_FLAG = 5,
        COUNT
    };

    WarSceneOrder(WarOrder order) : Order(to_uint(order))
    {
    }

    void evaluatePriority();
    void lock() { Order::lock(); }
    void unlock() { Order::unlock(); }
};

struct GOAPPackage {
    GOAPWorldState _worldState;
    GOAPGoalList _goalList;
    vectorImpl<WarSceneAction> _actionSet;
};

namespace Attorney {
    class WarAISceneWarAction;
};

class WarSceneAISceneImpl : public AISceneImpl {
    friend class Attorney::WarAISceneWarAction;
   public:
       typedef hashMapImpl<I64, AIEntity*> NodeToUnitMap;
   public:
       enum class AIType {
           ANIMAL = 0,
           LIGHT = 1,
           HEAVY = 2,
           COUNT
       };
    WarSceneAISceneImpl(AIType type);
    ~WarSceneAISceneImpl();

    void registerGOAPPackage(const GOAPPackage& package);

    bool processData(const U64 deltaTime);
    bool processInput(const U64 deltaTime);
    bool update(const U64 deltaTime, NPC* unitRef = nullptr);
    void processMessage(AIEntity& sender, AIMsg msg, const cdiggins::any& msg_content);

    static void registerFlags(std::weak_ptr<SceneGraphNode> flag1,
                              std::weak_ptr<SceneGraphNode> flag2) {
        _globalWorkingMemory._flags[0].value(flag1);
        _globalWorkingMemory._flags[1].value(flag2);
    }

    static void registerScoreCallback(const DELEGATE_CBK_PARAM<U8>& cbk) {
        _scoreCallback = cbk;
    }

    static U8 getScore(U8 teamID) {
        return _globalWorkingMemory._score[teamID].value();
    }

    static void reset();
    static void incrementScore(U8 teamID) {
        _globalWorkingMemory._score[teamID].value(getScore(teamID) + 1);
    }

   protected:
    bool preAction(ActionType type, const WarSceneAction* warAction);
    bool postAction(ActionType type, const WarSceneAction* warAction);
    bool checkCurrentActionComplete(const GOAPAction& planStep);
    stringImpl toString() const;

   private:
    bool DIE();
    void requestOrders();
    void updatePositions();
    bool performAction(const GOAPAction& planStep);
    bool performActionStep(GOAPAction::operationsIterator step);
    bool printActionStats(const GOAPAction& planStep) const;
    void printWorkingMemory() const;
    void initInternal();
    void beginPlan(const GOAPGoal& currentGoal);
    AIEntity* getUnitForNode(U32 teamID, std::weak_ptr<SceneGraphNode> node) const;

    template <typename... T>
    inline void PRINT(const char* format, T&&... args) const {
        #if defined(PRINT_AI_TO_FILE)
        Console::d_printfn(_WarAIOutputStream, format, std::forward<T>(args)...);
        #endif
    }

    bool atHomeBase() const;
    bool nearOwnFlag() const;
    bool nearEnemyFlag() const;

  private:
    AIType _type;
    U16 _tickCount;
    U64 _deltaTime;
    U8 _visualSensorUpdateCounter;
    U64 _attackTimer;
    stringImpl _planStatus;
    VisualSensor* _visualSensor;
    AudioSensor* _audioSensor;
    LocalWorkingMemory _localWorkingMemory;
    /// Keep this in memory at this level
    vectorImpl<WarSceneAction> _actionList;
    NodeToUnitMap _nodeToUnitMap[2];
    std::array<bool, to_const_uint(ActionType::COUNT)> _actionState;
    static DELEGATE_CBK_PARAM<U8> _scoreCallback;
    static GlobalWorkingMemory _globalWorkingMemory;
    static vec3<F32> _initialFlagPositions[2];

#if defined(PRINT_AI_TO_FILE)
    mutable std::ofstream _WarAIOutputStream;
#endif
};

namespace Attorney {
class WarAISceneWarAction {
   private:
    static bool preAction(WarSceneAISceneImpl& aiScene, ActionType type,
                          const Divide::AI::WarSceneAction* warAction) {
        return aiScene.preAction(type, warAction);
    }
    static bool postAction(WarSceneAISceneImpl& aiScene, ActionType type,
                           const Divide::AI::WarSceneAction* warAction) {
        return aiScene.postAction(type, warAction);
    }

    friend class Divide::AI::WarSceneAction;
};

};  // namespace Attorney
};  // namespace AI
};  // namespace Divide

#endif

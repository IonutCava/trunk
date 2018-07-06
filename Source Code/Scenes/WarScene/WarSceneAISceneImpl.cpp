#include "Headers/WarSceneAISceneImpl.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
namespace AI {

static const D32 g_ATTACK_RADIUS = 5;
static const D32 g_ATTACK_RADIUS_SQ = g_ATTACK_RADIUS * g_ATTACK_RADIUS;
static const U32 g_myTeamContainer = 0;
static const U32 g_enemyTeamContainer = 1;
static const U32 g_flagContainer = 2;
vec3<F32> WarSceneAISceneImpl::_initialFlagPositions[2];
GlobalWorkingMemory WarSceneAISceneImpl::_globalWorkingMemory;

WarSceneAISceneImpl::WarSceneAISceneImpl(AIType type)
    : AISceneImpl(),
      _type(type),
      _tickCount(0),
      _orderRequestTryCount(0),
      _visualSensorUpdateCounter(0),
      _deltaTime(0ULL),
      _visualSensor(nullptr),
      _audioSensor(nullptr)
{
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
    MemoryManager::DELETE_VECTOR(actionSetPtr());
    _WarAIOutputStream.close();
}

void WarSceneAISceneImpl::registerAction(GOAPAction* const action) {
    WarSceneAction* const warAction = static_cast<WarSceneAction*>(action);
    Attorney::WarSceneActionWarAIScene::setParentAIScene(*warAction, this);

    switch (warAction->actionType()) {
        case ActionType::ACTION_IDLE:
            AISceneImpl::registerAction(
                MemoryManager_NEW Idle(*warAction));
            return;
        case ActionType::ACTION_APPROACH_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW ApproachFlag(*warAction));
            return;
        case ActionType::ACTION_CAPTURE_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW CaptureFlag(*warAction));
            return;
        case ActionType::ACTION_SCORE_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW ScoreFlag(*warAction));
            return;
        case ActionType::ACTION_RETURN_FLAG_TO_BASE:
            AISceneImpl::registerAction(
                MemoryManager_NEW ReturnFlagHome(*warAction));
            return;
    };

    AISceneImpl::registerAction(MemoryManager_NEW WarSceneAction(*warAction));
}

void WarSceneAISceneImpl::initInternal() {
    _WarAIOutputStream.open(
        Util::stringFormat("AILogs/%s_.txt", _entity->getName().c_str()),
        std::ofstream::out | std::ofstream::trunc);
    _visualSensor =
        dynamic_cast<VisualSensor*>(
            _entity->getSensor(SensorType::VISUAL_SENSOR));
    //_audioSensor =
    // dynamic_cast<AudioSensor*>(_entity->getSensor(SensorType::AUDIO_SENSOR));
    DIVIDE_ASSERT(_visualSensor != nullptr,
                  "WarSceneAISceneImpl error: No visual sensor found for "
                  "current AI entity!");
    // DIVIDE_ASSERT(_audioSensor != nullptr, "WarSceneAISceneImpl error: No
    // audio sensor found for current AI entity!");

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        _visualSensor->followSceneGraphNode(
            g_myTeamContainer, *member.second->getUnitRef()->getBoundNode());
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        _visualSensor->followSceneGraphNode(
            g_enemyTeamContainer, *enemy.second->getUnitRef()->getBoundNode());
    }

    _visualSensor->followSceneGraphNode(
        g_flagContainer, *_globalWorkingMemory._flags[0].value());
    _visualSensor->followSceneGraphNode(
        g_flagContainer, *_globalWorkingMemory._flags[1].value());

    _initialFlagPositions[0].set(_globalWorkingMemory._flags[0]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());
    _initialFlagPositions[1].set(_globalWorkingMemory._flags[1]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());
}

void WarSceneAISceneImpl::requestOrders() {
    const vectorImpl<Order*>& orders = _entity->getTeam()->requestOrders();
    std::array<U8, to_const_uint(WarSceneOrder::WarOrder::COUNT)> priority = { 0 };

    resetActiveGoals();
    printWorkingMemory();

    for (Order* const order : orders) {
        WarSceneOrder::WarOrder orderID = static_cast<WarSceneOrder::WarOrder>(
            dynamic_cast<WarSceneOrder*>(order)->getID());
        U32 weight = 0;
        switch (orderID) {
            case WarSceneOrder::WarOrder::ORDER_CAPTURE_ENEMY_FLAG: {
                weight = 1;
            } break;
            case WarSceneOrder::WarOrder::ORDER_IDLE: {
                const AITeam* const currentTeam = _entity->getTeam();
                if (_globalWorkingMemory._score[currentTeam->getTeamID()].value() == 1) {
                    weight = 2;
                }
                // If we have the enemy flag and the enemy has our flag,
                // we MUST wait for someone else to retrieve it
                if (_localWorkingMemory._hasEnemyFlag.value() && 
                    _localWorkingMemory._enemyHasFlag.value()) {
                    // Invalidate everything else and make sure we IDLE
                    weight = 255;
                }
            } break;
            case WarSceneOrder::WarOrder::ORDER_SCORE_ENEMY_FLAG: {
                if (_localWorkingMemory._hasEnemyFlag.value()) {
                    weight = 3;
                    if (worldState().getVariable(GOAPFact(Fact::AtHomeFlagLoc)) == GOAPValue(true)) {
                        weight = 4;
                    }
                }
            } break;
        }
        priority[to_uint(orderID)] = weight;
    }

    for (GOAPGoal& goal : goalList()) {
        switch (static_cast<WarSceneOrder::WarOrder>(goal.getID())) {
            case WarSceneOrder::WarOrder::ORDER_CAPTURE_ENEMY_FLAG:
            {
                // add extra relevancy calculations here
                goal.relevancy(priority[goal.getID()]);
            } break;
            case WarSceneOrder::WarOrder::ORDER_SCORE_ENEMY_FLAG:
            {
                goal.relevancy(priority[goal.getID()]);
            } break;
            case WarSceneOrder::WarOrder::ORDER_IDLE:
            {
                goal.relevancy(priority[goal.getID()]);
            } break;
        };
    }

    GOAPGoal* goal = findRelevantGoal();

    if (goal != nullptr) {
        Console::d_printfn(_WarAIOutputStream, "Current goal: [ %s ]",
                           goal->getName().c_str());

        // Hack to loop in idle
        if (goal->getID() == to_uint(WarSceneOrder::WarOrder::ORDER_IDLE)) {
            worldState().setVariable(GOAPFact(Fact::Idling), GOAPValue(false));
        }

        if (replanGoal()) {
            Console::d_printfn(_WarAIOutputStream,
                               "The plan for goal [ %s ] involves [ %d ] actions.",
                               goal->getName().c_str(),
                               getActiveGoal()->getCurrentPlan().size());
            printPlan();
        } else {
            Console::d_printfn(_WarAIOutputStream, getPlanLog().c_str());
            Console::printfn(_WarAIOutputStream,
                             "Can't generate plan for goal [ %s ]",
                             goal->getName().c_str());
        }
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type,
                                    const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    switch (type) {
        case ActionType::ACTION_IDLE: {
            Console::d_printfn(_WarAIOutputStream, "Starting idle action");
            _entity->stop();
        } break;
        case ActionType::ACTION_APPROACH_FLAG: {
            Console::d_printfn(_WarAIOutputStream, "Starting approach flag action");
            _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value());
        } break;
        case ActionType::ACTION_CAPTURE_FLAG: {
            Console::d_printfn(_WarAIOutputStream, "Starting capture flag action");
        } break;
        case ActionType::ACTION_SCORE_FLAG: {
            Console::d_printfn(_WarAIOutputStream, "Starting score action");
        } break;
        case ActionType::ACTION_RETURN_FLAG_TO_BASE: {
            Console::d_printfn(_WarAIOutputStream, "Starting return flag to base action");
            _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[ownTeamID].value());
        } break;
        default: {
            assert(false);
        } break;
    };

    return true;
}

bool WarSceneAISceneImpl::postAction(ActionType type,
                                     const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    switch (type) {
        case ActionType::ACTION_IDLE: {
            Console::d_printfn(_WarAIOutputStream, "Idle action over");
        } break;
        case ActionType::ACTION_APPROACH_FLAG: {
            Console::d_printfn(_WarAIOutputStream, "Approach flag action over");
        } break;
        case ActionType::ACTION_SCORE_FLAG: {
            if (_localWorkingMemory._hasEnemyFlag.value()) {
                U8 score = _globalWorkingMemory._score[ownTeamID].value();
                _globalWorkingMemory._score[ownTeamID].value(score + 1);
                _localWorkingMemory._hasEnemyFlag.value(false);
                _globalWorkingMemory._flags[enemyTeamID].value()->setParent(GET_ACTIVE_SCENEGRAPH().getRoot());
                PhysicsComponent* pComp =
                    _globalWorkingMemory._flags[enemyTeamID]
                        .value()
                        ->getComponent<PhysicsComponent>();
                pComp->popTransforms();
                for (const AITeam::TeamMap::value_type& member :
                     currentTeam->getTeamMembers()) {
                    _entity->sendMessage(member.second, AIMsg::HAVE_SCORED, _entity);
                }
            }
        } break;
        case ActionType::ACTION_CAPTURE_FLAG: {
            Console::d_printfn(_WarAIOutputStream, "Capture flag action over");
            assert(!_localWorkingMemory._hasEnemyFlag.value());
            PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                                      .value()->getComponent<PhysicsComponent>();
            vec3<F32> prevScale(pComp->getScale());
            vec3<F32> prevPosition(pComp->getPosition());

            SceneGraphNode* targetNode = _entity->getUnitRef()->getBoundNode();
            _globalWorkingMemory._flags[enemyTeamID].value()->setParent(*targetNode);
            pComp->pushTransforms();
            pComp->setPosition(vec3<F32>(0.0f, 0.75f, 1.5f));
            pComp->setScale(
                prevScale /
                targetNode->getComponent<PhysicsComponent>()->getScale());

            _localWorkingMemory._hasEnemyFlag.value(true);
            _globalWorkingMemory._flagCarriers[ownTeamID].value(targetNode);

            for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::HAVE_FLAG, _entity);
            }
            const AITeam* const enemyTeam =
                AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
            for (const AITeam::TeamMap::value_type& enemy :
                 enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, AIMsg::ENEMY_HAS_FLAG,
                                     _entity);
            }
        } break;
        case ActionType::ACTION_RETURN_FLAG_TO_BASE: {
            Console::printfn(_WarAIOutputStream, "Return flag to base action over");
          
        } break;
        default: {
            assert(false);
        } break;
    };

    GOAPAction::operationsIterator o;
    for (o = std::begin(warAction->effects()); o != std::end(warAction->effects()); ++o) {
        performActionStep(o);
    }

    Console::printf(_WarAIOutputStream, "\n");

    return advanceGoal();
}

bool WarSceneAISceneImpl::checkCurrentActionComplete(const GOAPAction& planStep) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    const SceneGraphNode& currentNode = *(_entity->getUnitRef()->getBoundNode());
    const SceneGraphNode& enemyFlag = *(_globalWorkingMemory._flags[enemyTeamID].value());

    const WarSceneAction& warAction = static_cast<const WarSceneAction&>(planStep);

    bool state = false;

    const BoundingBox& bb1 = currentNode.getBoundingBoxConst();
    switch (warAction.actionType()) {
        case ActionType::ACTION_APPROACH_FLAG:
        case ActionType::ACTION_CAPTURE_FLAG: {
            state = enemyFlag.getBoundingBoxConst().Collision(bb1);
            // Not close enough. give a slight nudge
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(
                    _globalWorkingMemory._teamFlagPosition[enemyTeamID]
                        .value());
            }
        } break;
        case ActionType::ACTION_IDLE: {
            state = true;
        } break;
        case ActionType::ACTION_SCORE_FLAG:
            if (_localWorkingMemory._enemyHasFlag.value()) {
                invalidateCurrentPlan();
                return false;
            }
        case ActionType::ACTION_RETURN_FLAG_TO_BASE: {
            state = _entity->getPosition().distanceSquared(
                _initialFlagPositions[ownTeamID]) < g_ATTACK_RADIUS_SQ;
   
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(_initialFlagPositions[ownTeamID]);
            }
        } break;
        default: {
            assert(false);
        } break;
    };

    if (state) {
        return warAction.postAction();
    }
    return state;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg,
                                         const cdiggins::any& msg_content) {
    switch (msg) {
        case AIMsg::HAVE_FLAG: {
        } break;
        case AIMsg::ENEMY_HAS_FLAG: {
            _localWorkingMemory._enemyHasFlag.value(true);
            worldState().setVariable(AI::GOAPFact(Fact::EnemyHasFlag),
                                     AI::GOAPValue(true));
        } break;
    };
}

void WarSceneAISceneImpl::updatePositions() {
    if (_globalWorkingMemory._flags[0].value() != nullptr &&
        _globalWorkingMemory._flags[1].value() != nullptr) {
        _globalWorkingMemory._teamFlagPosition[0].value(
            _visualSensor->getNodePosition(
                g_flagContainer,
                _globalWorkingMemory._flags[0].value()->getGUID()));
        _globalWorkingMemory._teamFlagPosition[1].value(
            _visualSensor->getNodePosition(
                g_flagContainer,
                _globalWorkingMemory._flags[1].value()->getGUID()));
    }
}

void WarSceneAISceneImpl::processInput(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    _deltaTime += deltaTime;
    // wait 1 and a half seconds at the destination
    if (_deltaTime > Time::SecondsToMicroseconds(1.5)) {
        _deltaTime = 0;
    }
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    if (!AIManager::getInstance().getNavMesh(
            _entity->getAgentRadiusCategory())) {
        return;
    }

    updatePositions();

    if (!getActiveGoal()) {
        if (!goalList().empty() && _orderRequestTryCount < 3) {
            requestOrders();
            _orderRequestTryCount++;
        }
    } else {
        _orderRequestTryCount = 0;
        checkCurrentActionComplete(*getActiveAction());
    }
}

void WarSceneAISceneImpl::update(const U64 deltaTime, NPC* unitRef) {
    U8 visualSensorUpdateFreq = 10;
    _visualSensorUpdateCounter =
        (_visualSensorUpdateCounter + 1) % (visualSensorUpdateFreq + 1);
    // Update sensor information
    if (_visualSensor && _visualSensorUpdateCounter == visualSensorUpdateFreq) {
        _visualSensor->update(deltaTime);
    }
    if (_audioSensor) {
        _audioSensor->update(deltaTime);
    }
}

bool WarSceneAISceneImpl::performAction(const GOAPAction* planStep) {
    if (planStep == nullptr) {
        return false;
    }
    return static_cast<const WarSceneAction*>(planStep)->preAction();
}

bool WarSceneAISceneImpl::performActionStep(
    GOAPAction::operationsIterator step) {
    GOAPFact crtFact = step->first;
    GOAPValue newVal = step->second;
    GOAPValue oldVal = worldState().getVariable(crtFact);
    if (oldVal != newVal) {
        Console::printfn(_WarAIOutputStream, "\t\t Changing \"%s\" from \"%s\" to \"%s\"",
                         WarSceneFactName(crtFact),
                         GOAPValueName(oldVal),
                         GOAPValueName(newVal));
    }
    worldState().setVariable(crtFact, newVal);
    return true;
}

bool WarSceneAISceneImpl::printActionStats(const GOAPAction* planStep) const {
    Console::printfn(_WarAIOutputStream, "Action [ %s ]", planStep->name().c_str());
    return true;
}

void WarSceneAISceneImpl::printWorkingMemory() const {
    Console::printfn(_WarAIOutputStream,
        "--------------- Working memory state BEGIN ----------------------------");
    Console::printfn(_WarAIOutputStream, "        Current position: - [ %4.1f , %4.1f, %4.1f]",
                     _entity->getPosition().x, _entity->getPosition().y,
                     _entity->getPosition().z);
    Console::printfn(_WarAIOutputStream,
        "        Flag Positions - 0 : [ %4.1f , %4.1f, %4.1f] | 1 : [ %4.1f , "
        "%4.1f, %4.1f]",
        _globalWorkingMemory._teamFlagPosition[0].value().x,
        _globalWorkingMemory._teamFlagPosition[0].value().y,
        _globalWorkingMemory._teamFlagPosition[0].value().z,
        _globalWorkingMemory._teamFlagPosition[1].value().x,
        _globalWorkingMemory._teamFlagPosition[1].value().y,
        _globalWorkingMemory._teamFlagPosition[1].value().z);
    Console::printfn(_WarAIOutputStream, "        Has enemy flag: [ %s ]",
                     _localWorkingMemory._hasEnemyFlag.value() ? "true" : "false");
    for (std::pair<GOAPFact, GOAPValue> var : worldStateConst().vars_) {
        Console::printfn(_WarAIOutputStream, "        World state fact [ %s ] : [ %s ]",
                         WarSceneFactName(var.first),
                         var.second ? "true" : "false");
    }
    Console::printfn(_WarAIOutputStream,
        "--------------- Working memory state END ----------------------------");
}
};  // namespace AI
};  // namespace Divide

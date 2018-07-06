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
      _attackTimer(0ULL),
      _deltaTime(0ULL),
      _visualSensor(nullptr),
      _audioSensor(nullptr)
{
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
    _WarAIOutputStream.close();
}

void WarSceneAISceneImpl::initInternal() {
    _WarAIOutputStream.open(
        Util::stringFormat("AILogs/%s_.txt", _entity->getName().c_str()),
        std::ofstream::out | std::ofstream::trunc);
    _visualSensor =
        dynamic_cast<VisualSensor*>(
            _entity->getSensor(SensorType::VISUAL_SENSOR));
    DIVIDE_ASSERT(_visualSensor != nullptr,
                  "WarSceneAISceneImpl error: No visual sensor found for "
                  "current AI entity!");

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        SceneGraphNode* node = member.second->getUnitRef()->getBoundNode();
        _nodeToUnitMap[g_myTeamContainer].insert(
            hashAlg::makePair(node->getGUID(), member.second));
        _visualSensor->followSceneGraphNode(g_myTeamContainer, *node);
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        SceneGraphNode* node = enemy.second->getUnitRef()->getBoundNode();
        _nodeToUnitMap[g_enemyTeamContainer].insert(
            hashAlg::makePair(node->getGUID(), enemy.second));
        _visualSensor->followSceneGraphNode(g_enemyTeamContainer, *node);
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

    
    _globalWorkingMemory._teamAliveCount[g_myTeamContainer].value(static_cast<U8>(teamAgents.size()));
    _globalWorkingMemory._teamAliveCount[g_enemyTeamContainer].value(static_cast<U8>(enemyMembers.size()));
}

bool WarSceneAISceneImpl::DIE() {
    if (_entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) == 0) {
        return false;
    }

    _entity->getUnitRef()->setAttribute(to_uint(UnitAttributes::ALIVE_FLAG), 0);
    U8 teamCount = _globalWorkingMemory._teamAliveCount[g_enemyTeamContainer].value();
    _globalWorkingMemory._teamAliveCount[g_enemyTeamContainer].value(std::max(teamCount - 1, 0));

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        _entity->sendMessage(member.second, AIMsg::HAVE_DIED, _entity);
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        _entity->sendMessage(enemy.second, AIMsg::HAVE_DIED, _entity);
    }

    currentTeam->removeTeamMember(_entity);

    _entity->getUnitRef()->getBoundNode()->setActive(false);

    return true;
}

AIEntity* WarSceneAISceneImpl::getUnitForNode(U32 teamID, SceneGraphNode* node) const {
    if (node) {
        NodeToUnitMap::const_iterator it = _nodeToUnitMap[g_enemyTeamContainer].find(node->getGUID());
        if (it != std::end(_nodeToUnitMap[g_enemyTeamContainer])) {
            return it->second;
        }
    }

    return nullptr;
}

void WarSceneAISceneImpl::requestOrders() {
    const AITeam::OrderList& orders = _entity->getTeam()->requestOrders();
    std::array<U8, to_const_uint(WarSceneOrder::WarOrder::COUNT)> priority = { 0 };

    resetActiveGoals();
    printWorkingMemory();

    const AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    for (const AITeam::OrderPtr& order : orders) {
        WarSceneOrder::WarOrder orderID =
            static_cast<WarSceneOrder::WarOrder>(order->getID());
        U32 weight = 0;
        switch (orderID) {
            case WarSceneOrder::WarOrder::ORDER_CAPTURE_ENEMY_FLAG: {
                if (!_localWorkingMemory._hasEnemyFlag.value()) {
                    weight = 3;
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

            case WarSceneOrder::WarOrder::ORDER_IDLE: {
                weight = 1;
                const AITeam* const currentTeam = _entity->getTeam();
                // If we have the enemy flag and the enemy has our flag,
                // we MUST wait for someone else to retrieve it
                if (worldState().getVariable(GOAPFact(Fact::AtHomeFlagLoc)) == GOAPValue(true) &&
                    _localWorkingMemory._hasEnemyFlag.value() && 
                    _localWorkingMemory._enemyHasFlag.value()) {
                    weight = 5;
                }
            } break;

            case WarSceneOrder::WarOrder::ORDER_KILL_ENEMY: {
                if (_globalWorkingMemory._teamAliveCount[enemyTeamID].value() > 0) {
                    weight = 2;
                }
                if (_localWorkingMemory._currentTarget.value() != nullptr) {
                    weight = 6;
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
            case WarSceneOrder::WarOrder::ORDER_KILL_ENEMY:
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
        case ActionType::ACTION_ATTACK_ENEMY: {
            Console::d_printfn(_WarAIOutputStream, "Starting attack enemy action");
            if (_localWorkingMemory._currentTarget.value() == nullptr) {
                SceneGraphNode* enemy = _visualSensor->getClosestNode(g_enemyTeamContainer);
                assert(enemy);
                _entity->updateDestination(enemy->getComponent<PhysicsComponent>()->getPosition(), true);
                _localWorkingMemory._currentTarget.value(enemy);
            }
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
        case ActionType::ACTION_ATTACK_ENEMY: {
            Console::printfn(_WarAIOutputStream, "Attack enemy action over");
            _localWorkingMemory._currentTarget.value(nullptr);
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
        case ActionType::ACTION_ATTACK_ENEMY: {
            SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
            assert(enemy != nullptr);
            AIEntity* targetUnit = getUnitForNode(g_enemyTeamContainer, enemy);
            assert(targetUnit != nullptr);
            NPC* targetNPC = targetUnit->getUnitRef();
            state =  targetNPC->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) == 0;

             if (!state && _entity->destinationReached()) {
                _entity->updateDestination(targetNPC->getPosition());
            }
        } break;
        default: {
            assert(false);
        } break;
    };

    if (state) {
        return warAction.postAction(*this);
    }
    return state;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg,
                                         const cdiggins::any& msg_content) {
    switch (msg) {
        case AIMsg::HAVE_FLAG: {
            invalidateCurrentPlan();
        } break;
        case AIMsg::ENEMY_HAS_FLAG: {
            _localWorkingMemory._enemyHasFlag.value(true);
            worldState().setVariable(AI::GOAPFact(Fact::EnemyHasFlag),
                                     AI::GOAPValue(true));
        } break;
        case AIMsg::ATTACK: {
            if (!_localWorkingMemory._currentTarget.value() || 
                _localWorkingMemory._currentTarget.value()->getGUID() != sender->getUnitRef()->getBoundNode()->getGUID()) {
                invalidateCurrentPlan();
                _localWorkingMemory._currentTarget.value(sender->getUnitRef()->getBoundNode());
            }
        } break;

        case AIMsg::HAVE_DIED: {
             SceneGraphNode* node = sender->getUnitRef()->getBoundNode();
            _visualSensor->unfollowSceneGraphNode(g_myTeamContainer, node->getGUID());
            _visualSensor->unfollowSceneGraphNode(g_enemyTeamContainer, node->getGUID());
            if (_localWorkingMemory._currentTarget.value() != nullptr &&
                _localWorkingMemory._currentTarget.value()->getGUID() == node->getGUID()) {
                _localWorkingMemory._currentTarget.value(nullptr);
                invalidateCurrentPlan();
            }
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

    if (_entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::HEALTH_POINTS)) == 0) {
        DIE();
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

    _attackTimer += deltaTime;
    SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
    if (enemy != nullptr) {
        AIEntity* targetUnit = getUnitForNode(g_enemyTeamContainer, enemy);
        if (targetUnit != nullptr) {
            NPC* targetNPC = targetUnit->getUnitRef();
            bool alive = targetNPC->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) > 0;
            if (alive && _entity->getPosition().distanceSquared(targetNPC->getPosition()) < g_ATTACK_RADIUS_SQ) {
                _entity->getUnitRef()->lookAt(targetNPC->getPosition());
                
                if (_attackTimer >= Time::SecondsToMicroseconds(0.5)) {
                    I32 HP = targetNPC->getAttribute(to_uint(UnitAttributes::HEALTH_POINTS));
                    I32 Damage = _entity->getUnitRef()->getAttribute(to_uint(UnitAttributes::DAMAGE));
                    targetNPC->setAttribute(to_uint(UnitAttributes::HEALTH_POINTS), std::max(HP - Damage, 0));
                    _entity->sendMessage(targetUnit, AIMsg::ATTACK, 0);
                    _attackTimer = 0ULL;
                }
            } else {
                _attackTimer = 0ULL;
            }
        }
    }else {
        _attackTimer = 0ULL;
    }

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

bool WarSceneAISceneImpl::performAction(const GOAPAction& planStep) {
    return static_cast<const WarSceneAction&>(planStep).preAction(*this);
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

bool WarSceneAISceneImpl::printActionStats(const GOAPAction& planStep) const {
    Console::printfn(_WarAIOutputStream, "Action [ %s ]", planStep.name().c_str());
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
    for (std::pair<GOAPFact, GOAPValue> var : worldState().vars_) {
        Console::printfn(_WarAIOutputStream, "        World state fact [ %s ] : [ %s ]",
                         WarSceneFactName(var.first),
                         var.second ? "true" : "false");
    }
    Console::printfn(_WarAIOutputStream,
        "--------------- Working memory state END ----------------------------");
}

void WarSceneAISceneImpl::registerGOAPPackage(const GOAPPackage& package) {
    worldState() = package._worldState;
    registerGoalList(package._goalList);
    for (const WarSceneAction& action : package._actionSet) {
        _actionList.push_back(action);
    }
    for (const WarSceneAction& action : _actionList) {
        registerAction(action);
    }
}

};  // namespace AI
};  // namespace Divide

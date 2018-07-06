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
static const U32 g_myTeamContainer = 0;
static const U32 g_enemyTeamContainer = 1;
static const U32 g_flagContainer = 2;

PositionFact WorkingMemory::_teamFlagPosition[2];
SmallCounterFact WorkingMemory::_teamCount[2];
SGNNodeFact WorkingMemory::_flags[2];
SmallCounterFact WorkingMemory::_flagProtectors[2];
SmallCounterFact WorkingMemory::_flagRetrievers[2];
vec3<F32> WarSceneAISceneImpl::_initialFlagPositions[2];

WarSceneAISceneImpl::WarSceneAISceneImpl()
    : AISceneImpl(),
      _tickCount(0),
      _orderRequestTryCount(0),
      _visualSensorUpdateCounter(0),
      _deltaTime(0ULL),
      _visualSensor(nullptr),
      _audioSensor(nullptr) {}

WarSceneAISceneImpl::~WarSceneAISceneImpl() {
    MemoryManager::DELETE_VECTOR(actionSetPtr());
}

void WarSceneAISceneImpl::registerAction(GOAPAction* const action) {
    WarSceneAction* const warAction = static_cast<WarSceneAction*>(action);
    warAction->setParentAIScene(this);

    switch (warAction->actionType()) {
        case ACTION_APPROACH_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW ApproachFlag(*warAction));
            return;
        case ACTION_CAPTURE_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW CaptureFlag(*warAction));
            return;
        case ACTION_RETURN_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW ReturnFlag(*warAction));
            return;
        case ACTION_RECOVER_FLAG:
            AISceneImpl::registerAction(
                MemoryManager_NEW RecoverFlag(*warAction));
            return;
        case ACTION_KILL_ENEMY:
            AISceneImpl::registerAction(
                MemoryManager_NEW KillEnemy(*warAction));
            return;
        case ACTION_RETURN_TO_BASE:
            AISceneImpl::registerAction(
                MemoryManager_NEW ReturnHome(*warAction));
            return;
        case ACTION_PROTECT_FLAG_CARRIER:
            AISceneImpl::registerAction(
                MemoryManager_NEW ProtectFlagCarrier(*warAction));
            return;
    };

    AISceneImpl::registerAction(MemoryManager_NEW WarSceneAction(*warAction));
}

void WarSceneAISceneImpl::init() {
    _visualSensor =
        dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    //_audioSensor =
    //dynamic_cast<AudioSensor*>(_entity->getSensor(AUDIO_SENSOR));
    DIVIDE_ASSERT(_visualSensor != nullptr,
                  "WarSceneAISceneImpl error: No visual sensor found for "
                  "current AI entity!");
    // DIVIDE_ASSERT(_audioSensor != nullptr, "WarSceneAISceneImpl error: No
    // audio sensor found for current AI entity!");

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam =
        AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        _visualSensor->followSceneGraphNode(
            g_myTeamContainer, member.second->getUnitRef()->getBoundNode());
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        _visualSensor->followSceneGraphNode(
            g_enemyTeamContainer, enemy.second->getUnitRef()->getBoundNode());
    }

    _visualSensor->followSceneGraphNode(g_flagContainer,
                                        WorkingMemory::_flags[0].value());
    _visualSensor->followSceneGraphNode(g_flagContainer,
                                        WorkingMemory::_flags[1].value());

    _initialFlagPositions[0].set(WorkingMemory::_flags[0]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());
    _initialFlagPositions[1].set(WorkingMemory::_flags[1]
                                     .value()
                                     ->getComponent<PhysicsComponent>()
                                     ->getPosition());
}

void WarSceneAISceneImpl::requestOrders() {
    const vectorImpl<Order*>& orders = _entity->getTeam()->requestOrders();
    U8 priority[WarSceneOrder::ORDER_PLACEHOLDER];
    memset(priority, 0, sizeof(U8) * WarSceneOrder::ORDER_PLACEHOLDER);

    resetActiveGoals();
    printWorkingMemory();

    for (Order* const order : orders) {
        WarSceneOrder::WarOrder orderId = static_cast<WarSceneOrder::WarOrder>(
            dynamic_cast<WarSceneOrder*>(order)->getId());
        switch (orderId) {
            case WarSceneOrder::ORDER_FIND_ENEMY_FLAG: {
                if (!_workingMemory._hasEnemyFlag.value() &&
                    !_workingMemory._enemyFlagNear.value() &&
                    !_workingMemory._teamMateHasFlag.value()) {
                    priority[orderId] = 240;
                }
            } break;
            case WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG: {
                if (!_workingMemory._hasEnemyFlag.value() &&
                    _workingMemory._enemyFlagNear.value() &&
                    !_workingMemory._teamMateHasFlag.value()) {
                    priority[orderId] = 245;
                }
            } break;
            case WarSceneOrder::ORDER_RETURN_ENEMY_FLAG: {
                if (_workingMemory._hasEnemyFlag.value()) {
                    priority[orderId] = 250;
                }
            } break;
            case WarSceneOrder::ORDER_PROTECT_FLAG_CARRIER: {
                if (!_workingMemory._hasEnemyFlag.value() &&
                    _workingMemory._teamMateHasFlag.value()) {
                    priority[orderId] =
                        WorkingMemory::_flagProtectors[_entity->getTeamID()]
                                    .value() > 2
                            ? 125
                            : 220;
                }
            } break;
            case WarSceneOrder::ORDER_RETRIEVE_FLAG: {
                if (_workingMemory._enemyHasFlag.value() &&
                    !_workingMemory._hasEnemyFlag.value()) {
                    priority[orderId] =
                        WorkingMemory::_flagRetrievers[_entity->getTeamID()]
                                    .value() > 1
                            ? 125
                            : 200;
                    if (_workingMemory._friendlyFlagNear.value()) {
                        priority[orderId] += 2;
                    }
                    if (_workingMemory._enemyFlagNear.value()) {
                        priority[orderId] -= 2;
                    }
                }
            } break;
        }
    }

    for (GOAPGoal& goal : goalList()) {
        if (goal.getName().compare("Find enemy flag") == 0) {
            goal.relevancy(
                std::max(priority[WarSceneOrder::ORDER_FIND_ENEMY_FLAG],
                         priority[WarSceneOrder::ORDER_PROTECT_FLAG_CARRIER]));
        }
        if (goal.getName().compare("Capture enemy flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG]);
        }
        if (goal.getName().compare("Return enemy flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_RETURN_ENEMY_FLAG]);
        }
        if (goal.getName().compare("Retrieve flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_RETRIEVE_FLAG]);
        }
    }
    GOAPGoal* goal = findRelevantGoal();
    if (goal != nullptr) {
        Console::d_printfn("Current goal for [%d ]: [ %s ]", _entity->getGUID(),
                           goal->getName().c_str());
        if (replanGoal()) {
            Console::d_printfn("The Plan for [ %d ] involves [ %d ] actions.",
                               _entity->getGUID(),
                               getActiveGoal()->getCurrentPlan().size());
            printPlan();
        } else {
            Console::printfn("Invalid [ %d ]", _entity->getGUID());
        }
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type,
                                    const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (type) {
        case ACTION_APPROACH_FLAG: {
            Console::d_printfn("Starting approach flag action [ %d ]",
                               _entity->getGUID());
            _entity->updateDestination(
                _workingMemory._teamFlagPosition[1 - currentTeam->getTeamID()]
                    .value());
        } break;
        case ACTION_CAPTURE_FLAG: {
            Console::d_printfn("Starting capture flag action [ %d ]",
                               _entity->getGUID());
        } break;
        case ACTION_RETURN_TO_BASE: {
            Console::d_printfn("Starting return to base action [ %d ]",
                               _entity->getGUID());
            _entity->updateDestination(
                _workingMemory._teamFlagPosition[currentTeam->getTeamID()]
                    .value());
        } break;
        case ACTION_RETURN_FLAG: {
            Console::d_printfn("Starting return flag action [ %d ]",
                               _entity->getGUID());
        } break;
        case ACTION_PROTECT_FLAG_CARRIER: {
            Console::d_printfn("Starting protect flag action [ %d ]",
                               _entity->getGUID());
            assert(_workingMemory._flagCarrier.value() != nullptr);
            _entity->updateDestination(
                _workingMemory._flagCarrier.value()->getPosition());
            U8 temp = WorkingMemory::_flagProtectors[currentTeam->getTeamID()]
                          .value() +
                      1;
            WorkingMemory::_flagProtectors[currentTeam->getTeamID()].value(
                temp);
        } break;
        case ACTION_RECOVER_FLAG: {
            Console::d_printfn("Starting recover flag action [ %d ]",
                               _entity->getGUID());
            assert(_workingMemory._enemyFlagCarrier.value() != nullptr);
            _entity->updateDestination(
                _workingMemory._enemyFlagCarrier.value()->getPosition());
            U8 temp = WorkingMemory::_flagRetrievers[currentTeam->getTeamID()]
                          .value() +
                      1;
            WorkingMemory::_flagRetrievers[currentTeam->getTeamID()].value(
                temp);
        } break;
        default: {
            Console::d_printfn("Starting normal action [ %d ]",
                               _entity->getGUID());
        } break;
    };

    return true;
}

bool WarSceneAISceneImpl::postAction(ActionType type,
                                     const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (type) {
        case ACTION_APPROACH_FLAG: {
            Console::d_printfn("Approach flag action over [ %d ]",
                               _entity->getGUID());
        } break;
        case ACTION_CAPTURE_FLAG: {
            Console::d_printfn("Capture flag action over [ %d ]",
                               _entity->getGUID());
            assert(!_workingMemory._hasEnemyFlag.value());
            assert(!_workingMemory._teamMateHasFlag.value());

            U8 flag = 1 - currentTeam->getTeamID();
            PhysicsComponent* pComp = _workingMemory._flags[flag]
                                          .value()
                                          ->getComponent<PhysicsComponent>();
            vec3<F32> prevScale(pComp->getScale());

            SceneGraphNode* targetNode = _entity->getUnitRef()->getBoundNode();
            _workingMemory._flags[flag].value()->setParent(targetNode);
            pComp->setPosition(vec3<F32>(0.0f, 0.75f, 1.5f));
            pComp->setScale(
                prevScale /
                targetNode->getComponent<PhysicsComponent>()->getScale());

            _workingMemory._hasEnemyFlag.value(true);
            for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, HAVE_FLAG, _entity);
            }
            const AITeam* const enemyTeam =
                AIManager::getInstance().getTeamByID(
                    currentTeam->getEnemyTeamID(0));
            for (const AITeam::TeamMap::value_type& enemy :
                 enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, ENEMY_HAS_FLAG, _entity);
            }
        } break;
        case ACTION_RETURN_TO_BASE: {
            Console::printfn("Return to base action over [ %d ]",
                             _entity->getGUID());
        } break;

        case ACTION_RETURN_FLAG: {
            Console::printfn("Return flag action over [ %d ]",
                             _entity->getGUID());

            U8 flag = 1 - currentTeam->getTeamID();
            PhysicsComponent* pComp = _workingMemory._flags[flag]
                                          .value()
                                          ->getComponent<PhysicsComponent>();
            vec3<F32> prevScale(pComp->getScale());

            SceneGraphNode* targetNode =
                _workingMemory._flags[flag].value()->getParent();
            _workingMemory._flags[flag].value()->setParent(
                GET_ACTIVE_SCENEGRAPH().getRoot());
            pComp->setPosition(_initialFlagPositions[flag]);
            pComp->setScale(
                prevScale *
                targetNode->getComponent<PhysicsComponent>()->getScale());
        } break;
        case ACTION_PROTECT_FLAG_CARRIER: {
            U8 temp = WorkingMemory::_flagProtectors[currentTeam->getTeamID()]
                          .value() -
                      1;
            WorkingMemory::_flagProtectors[currentTeam->getTeamID()].value(
                temp);
        } break;
        case ACTION_RECOVER_FLAG: {
            U8 temp = WorkingMemory::_flagRetrievers[currentTeam->getTeamID()]
                          .value() -
                      1;
            WorkingMemory::_flagRetrievers[currentTeam->getTeamID()].value(
                temp);
        } break;
        default: {
            Console::printfn("Normal action over [ %d ]", _entity->getGUID());
        } break;
    };

    Console::printfn("   %s [ %d ]", warAction->name().c_str(),
                     _entity->getGUID());
    for (GOAPAction::operationsIterator o = std::begin(warAction->effects());
         o != std::end(warAction->effects()); o++) {
        performActionStep(o);
    }
    Console::printf("\n");
    return advanceGoal();
}

bool WarSceneAISceneImpl::checkCurrentActionComplete(
    const GOAPAction* planStep) {
    assert(planStep != nullptr);
    const WarSceneAction& warAction =
        *static_cast<const WarSceneAction*>(planStep);

    if (_workingMemory._teamMateHasFlag.value() &&
        (warAction.actionType() == ACTION_APPROACH_FLAG ||
         warAction.actionType() == ACTION_CAPTURE_FLAG ||
         warAction.actionType() == ACTION_RETURN_FLAG)) {
        invalidateCurrentPlan();
        return false;
    }

    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");
    const SceneGraphNode* const currentNode =
        _entity->getUnitRef()->getBoundNode();
    const SceneGraphNode* const enemyFlag =
        _workingMemory._flags[1 - currentTeam->getTeamID()].value();

    bool state = false;
    const BoundingBox& bb1 = currentNode->getBoundingBoxConst();
    switch (warAction.actionType()) {
        case ACTION_APPROACH_FLAG: {
            state = enemyFlag->getBoundingBoxConst().Collision(bb1);
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(
                    _workingMemory
                        ._teamFlagPosition[1 - currentTeam->getTeamID()]
                        .value());
            }
        } break;
        case ACTION_CAPTURE_FLAG: {
            state = enemyFlag->getBoundingBoxConst().Collision(bb1);
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(
                    _workingMemory
                        ._teamFlagPosition[1 - currentTeam->getTeamID()]
                        .value());
            }
        } break;
        case ACTION_RETURN_FLAG: {
            const SceneGraphNode* const ownFlag =
                _workingMemory._flags[currentTeam->getTeamID()].value();
            const BoundingBox& ownFlagBB = ownFlag->getBoundingBoxConst();
            // Our flag is in its original position and the 2 flags are touching
            const vec3<F32>& flagPos =
                ownFlag->getComponent<PhysicsComponent>()->getPosition();
            state = (enemyFlag->getBoundingBoxConst().Collision(ownFlagBB) &&
                     flagPos.distanceSquared(
                         _initialFlagPositions[currentTeam->getTeamID()]) <
                         g_ATTACK_RADIUS);
        } break;
        case ACTION_PROTECT_FLAG_CARRIER: {
            if (_entity->destinationReached()) {
                _entity->updateDestination(
                    _workingMemory._flagCarrier.value()->getPosition(), true);
            }
        } break;
        case ACTION_RECOVER_FLAG: {
            if (_entity->destinationReached()) {
                _entity->updateDestination(
                    _workingMemory._enemyFlagCarrier.value()->getPosition());
            }
        } break;
        case ACTION_RETURN_TO_BASE: {
            state = _entity->getPosition().distanceSquared(
                        _initialFlagPositions[currentTeam->getTeamID()]) <
                    g_ATTACK_RADIUS * g_ATTACK_RADIUS;
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(
                    _initialFlagPositions[currentTeam->getTeamID()]);
            }
        } break;
        default: {
            Console::printfn("Normal action check [ %d ]", _entity->getGUID());
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
        case HAVE_FLAG: {
            _workingMemory._teamMateHasFlag.value(true);
            _workingMemory._flagCarrier.value(sender);
        } break;
        case ENEMY_HAS_FLAG: {
            _workingMemory._enemyHasFlag.value(true);
            _workingMemory._enemyFlagCarrier.value(sender);
            worldState().setVariable(GOAPFact(EnemyHasFlag), GOAPValue(true));
        } break;
    };

    if (msg == HAVE_FLAG || msg == ENEMY_HAS_FLAG) {
        invalidateCurrentPlan();
    }
}

void WarSceneAISceneImpl::updatePositions() {
    if (_workingMemory._flags[0].value() != nullptr &&
        _workingMemory._flags[1].value() != nullptr) {
        _workingMemory._teamFlagPosition[0].value(
            _visualSensor->getNodePosition(
                g_flagContainer, _workingMemory._flags[0].value()->getGUID()));
        _workingMemory._teamFlagPosition[1].value(
            _visualSensor->getNodePosition(
                g_flagContainer, _workingMemory._flags[1].value()->getGUID()));

        const vec3<F32>& flag1Pos =
            _workingMemory._teamFlagPosition[1 - _entity->getTeamID()].value();
        const vec3<F32>& flag2Pos =
            _workingMemory._teamFlagPosition[1 - _entity->getTeamID()].value();
        F32 radiusSq = g_ATTACK_RADIUS * g_ATTACK_RADIUS;
        _workingMemory._enemyFlagNear.value(
            flag1Pos.distanceSquared(_entity->getPosition()) < radiusSq);
        _workingMemory._friendlyFlagNear.value(
            flag2Pos.distanceSquared(_entity->getPosition()) < radiusSq);
    }

    if (_workingMemory._currentTargetEntity.value()) {
        PhysicsComponent* pComp = _workingMemory._currentTargetEntity.value()
                                      ->getComponent<PhysicsComponent>();
        _workingMemory._currentTargetPosition.value(pComp->getPosition());
    }
}

void WarSceneAISceneImpl::processInput(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    _deltaTime += deltaTime;

    if (_deltaTime >
        Time::SecondsToMicroseconds(
            1.5)) {  // wait 1 and a half seconds at the destination
        _deltaTime = 0;
    }
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }
    AIManager& aiMgr = AIManager::getInstance();

    if (!aiMgr.getNavMesh(_entity->getAgentRadiusCategory())) {
        return;
    }

    updatePositions();

    if (_workingMemory._staticDataUpdated) {
        if (!getActiveGoal()) {
            if (!goalList().empty() && _orderRequestTryCount < 3) {
                requestOrders();
                _orderRequestTryCount++;
            }
        } else {
            _orderRequestTryCount = 0;
            checkCurrentActionComplete(getActiveAction());
        }
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

    if (!_workingMemory._staticDataUpdated) {
        AITeam* team1 = AIManager::getInstance().getTeamByID(0);
        if (team1 != nullptr) {
            const AITeam::TeamMap& team1Members = team1->getTeamMembers();
            _workingMemory._teamCount[0].value(
                static_cast<U8>(team1Members.size()));
        }
        AITeam* team2 = AIManager::getInstance().getTeamByID(1);
        if (team2 != nullptr) {
            const AITeam::TeamMap& team2Members = team2->getTeamMembers();
            _workingMemory._teamCount[1].value(
                static_cast<U8>(team2Members.size()));
        }
        _workingMemory._staticDataUpdated = true;
    }

    if (!_workingMemory._currentTargetEntity.value()) {
        _workingMemory._currentTargetEntity.value(
            _visualSensor->getClosestNode(g_enemyTeamContainer));
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
        Console::printfn("\t\t [%d] Changing \"%s\" from \"%s\" to \"%s\"",
                         _entity->getGUID(), WarSceneFactName(crtFact),
                         GOAPValueName(oldVal), GOAPValueName(newVal));
    }
    worldState().setVariable(crtFact, newVal);
    return true;
}

bool WarSceneAISceneImpl::printActionStats(const GOAPAction* planStep) const {
    Console::printfn("Action [ %s ] [ %d ]", planStep->name().c_str(),
                     _entity->getGUID());
    return true;
}

void WarSceneAISceneImpl::printWorkingMemory() const {
    Console::printfn(
        "--------------- Working memory state for [ %d ] BEGIN "
        "----------------------------",
        _entity->getGUID());
    Console::printfn("        Team Counts - 0: %d | 1: %d",
                     _workingMemory._teamCount[0].value(),
                     _workingMemory._teamCount[1].value());
    Console::printfn("        Current position: - [ %4.1f , %4.1f, %4.1f]",
                     _entity->getPosition().x, _entity->getPosition().y,
                     _entity->getPosition().z);
    Console::printfn("        Flag Protectors - 0: %d | 1: %d",
                     _workingMemory._flagProtectors[0].value(),
                     _workingMemory._flagProtectors[1].value());
    Console::printfn("        Flag Retrievers - 0: %d | 1: %d",
                     _workingMemory._flagRetrievers[0].value(),
                     _workingMemory._flagRetrievers[1].value());
    Console::printfn(
        "        Flag Positions - 0 : [ %4.1f , %4.1f, %4.1f] | 1 : [ %4.1f , "
        "%4.1f, %4.1f]",
        _workingMemory._teamFlagPosition[0].value().x,
        _workingMemory._teamFlagPosition[0].value().y,
        _workingMemory._teamFlagPosition[0].value().z,
        _workingMemory._teamFlagPosition[1].value().x,
        _workingMemory._teamFlagPosition[1].value().y,
        _workingMemory._teamFlagPosition[1].value().z);

    if (_workingMemory._flagCarrier.value()) {
        Console::printfn("        Flag carrier : [ %d ] ",
                         _workingMemory._flagCarrier.value()->getGUID());
    }
    if (_workingMemory._enemyFlagCarrier.value()) {
        Console::printfn("        Enemy flag carrier : [ %d ] ",
                         _workingMemory._enemyFlagCarrier.value()->getGUID());
    }
    Console::printfn("        Health : [ %d ]", _workingMemory._health.value());
    if (_workingMemory._currentTargetEntity.value()) {
        Console::printfn(
            "        Current target : [ %d ] ",
            _workingMemory._currentTargetEntity.value()->getGUID());
    }
    Console::printfn(
        "        Current target position: - [ %4.1f , %4.1f, %4.1f]",
        _workingMemory._currentTargetPosition.value().x,
        _workingMemory._currentTargetPosition.value().y,
        _workingMemory._currentTargetPosition.value().z);
    Console::printfn("        Has enemy flag: [ %s ]",
                     _workingMemory._hasEnemyFlag.value() ? "true" : "false");
    Console::printfn("        Enemy has flag: [ %s ]",
                     _workingMemory._enemyHasFlag.value() ? "true" : "false");
    Console::printfn(
        "        Teammate has flag: [ %s ]",
        _workingMemory._teamMateHasFlag.value() ? "true" : "false");
    Console::printfn("        Enemy flag near: [ %s ]",
                     _workingMemory._enemyFlagNear.value() ? "true" : "false");
    Console::printfn(
        "        Friendly flag near: [ %s ]",
        _workingMemory._friendlyFlagNear.value() ? "true" : "false");

    for (std::pair<GOAPFact, GOAPValue> var : worldStateConst().vars_) {
        Console::printfn("        World state fact [ %s ] : [ %s ]",
                         WarSceneFactName(var.first),
                         var.second ? "true" : "false");
    }
    Console::printfn(
        "--------------- Working memory state for [ %d ] END "
        "----------------------------",
        _entity->getGUID());
}
};  // namespace AI
};  // namespace Divide

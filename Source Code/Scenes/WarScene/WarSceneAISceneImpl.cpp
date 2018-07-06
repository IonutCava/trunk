#include "Headers/WarSceneAISceneImpl.h"

#include "Headers/WarScene.h"

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
      _visualSensorUpdateCounter(0),
      _attackTimer(0ULL),
      _deltaTime(0ULL),
      _visualSensor(nullptr),
      _audioSensor(nullptr)
{
    _actionState.fill(false);
    _planStatus = "None";
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
#if defined(PRINT_AI_TO_FILE)
    _WarAIOutputStream.close();
#endif
}

void WarSceneAISceneImpl::reset() {
    _globalWorkingMemory._flags[0].value(nullptr);
    _globalWorkingMemory._flags[1].value(nullptr);
    _globalWorkingMemory._flagCarriers[0].value(nullptr);
    _globalWorkingMemory._flagCarriers[1].value(nullptr);
    _globalWorkingMemory._flagsAtBase[0].value(true);
    _globalWorkingMemory._flagsAtBase[1].value(true);
}

void WarSceneAISceneImpl::initInternal() {
#if defined(PRINT_AI_TO_FILE)
    _WarAIOutputStream.open(
        Util::stringFormat("AILogs/%s_.txt", _entity->getName().c_str()),
        std::ofstream::out | std::ofstream::trunc);
#endif

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
     
    AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    _entity->getUnitRef()->setAttribute(to_uint(UnitAttributes::ALIVE_FLAG), 0);
    U8 teamCount = _globalWorkingMemory._teamAliveCount[ownTeamID].value();
    _globalWorkingMemory._teamAliveCount[ownTeamID].value(std::max(teamCount - 1, 0));

    bool hadFlag = _localWorkingMemory._hasEnemyFlag.value();
    if (hadFlag == true) {
        _globalWorkingMemory._flags[enemyTeamID].value()->setParent(
        GET_ACTIVE_SCENEGRAPH().getRoot());
        PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                                  .value()
                                  ->getComponent<PhysicsComponent>();
         pComp->popTransforms();
         pComp->pushTransforms();
         pComp->setPosition(_entity->getPosition());
        _globalWorkingMemory._flagCarriers[ownTeamID].value(nullptr);
    }

    if (_localWorkingMemory._isFlagRetriever.value()) {
        _globalWorkingMemory._flagRetrieverCount[ownTeamID].value(
            _globalWorkingMemory._flagRetrieverCount[ownTeamID].value() -
            1);
    }

    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    for (const AITeam::TeamMap::value_type& member : teamAgents) {
        _entity->sendMessage(member.second, AIMsg::HAVE_DIED, hadFlag);
    }

    for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
        _entity->sendMessage(enemy.second, AIMsg::HAVE_DIED, hadFlag);
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

void WarSceneAISceneImpl::beginPlan(const GOAPGoal& currentGoal) {
    const AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();

    if (_localWorkingMemory._isFlagRetriever.value() == true) {
        _globalWorkingMemory._flagRetrieverCount[ownTeamID].value(
            _globalWorkingMemory._flagRetrieverCount[ownTeamID].value() -
            1);
        _localWorkingMemory._isFlagRetriever.value(false);
    }
    
    switch (static_cast<WarSceneOrder::WarOrder>(currentGoal.getID())) {
        case WarSceneOrder::WarOrder::RECOVER_FLAG: {
            _localWorkingMemory._isFlagRetriever.value(true);
            _globalWorkingMemory._flagRetrieverCount[ownTeamID].value(
                _globalWorkingMemory._flagRetrieverCount[ownTeamID].value() +
                1);
        } break;
    };
    printPlan();
}

namespace {
    enum class PriorityLevel : U32 {
        MAX_PRIORITY = 5,
        VERY_HIGH_PRIORITY = 4,
        HIGH_PRIORITY = 3,
        MEDIUM_PRIORITY = 2,
        LOW_PRIORITY = 1,
        IGNORE_PRIORITY = 0
    };
};

void WarSceneAISceneImpl::requestOrders() {
    const AITeam::OrderList& orders = _entity->getTeam()->requestOrders();
    std::array<U8, to_const_uint(WarSceneOrder::WarOrder::COUNT)> priority = { 0 };

    resetActiveGoals();
    printWorkingMemory();

    const AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    PriorityLevel prio;

    bool atHome = atHomeBase();
    bool nearEnemyF = nearEnemyFlag();
    WarSceneOrder::WarOrder orderID;
    for (const AITeam::OrderPtr& order : orders) {
        orderID = static_cast<WarSceneOrder::WarOrder>(order->getID());

        prio = PriorityLevel::IGNORE_PRIORITY;
        switch (orderID) {
            case WarSceneOrder::WarOrder::CAPTURE_ENEMY_FLAG: {
                if (_globalWorkingMemory._flagCarriers[ownTeamID].value() == nullptr) {
                    prio = PriorityLevel::MAX_PRIORITY;
                }
            } break;

            case WarSceneOrder::WarOrder::SCORE_ENEMY_FLAG: {
                if (_localWorkingMemory._hasEnemyFlag.value() == true &&
                    _globalWorkingMemory._flagsAtBase[ownTeamID].value() == true) {
                    assert(_globalWorkingMemory._flagCarriers[ownTeamID].value()->getGUID() == _entity->getGUID());
                    prio = PriorityLevel::MAX_PRIORITY;
                }
            } break;

            case WarSceneOrder::WarOrder::IDLE: {
                prio = PriorityLevel::LOW_PRIORITY;
                // If we have the enemy flag and the enemy has our flag,
                // we MUST wait for someone else to retrieve it
                if (atHome &&
                    _localWorkingMemory._hasEnemyFlag.value() == true && 
                    _globalWorkingMemory._flagsAtBase[ownTeamID].value() == false &&
                    _globalWorkingMemory._teamAliveCount[ownTeamID].value() > 1){
                     prio = PriorityLevel::MAX_PRIORITY;
                }
            } break;

            case WarSceneOrder::WarOrder::KILL_ENEMY: {
                if (_localWorkingMemory._currentTarget.value() != nullptr) {
                    prio = PriorityLevel::VERY_HIGH_PRIORITY;
                }
                if (_globalWorkingMemory._teamAliveCount[enemyTeamID].value() > 0) {
                    prio = PriorityLevel::MEDIUM_PRIORITY;
                }
            } break;

            case WarSceneOrder::WarOrder::PROTECT_FLAG_CARRIER: {
                if (_globalWorkingMemory._flagCarriers[ownTeamID].value() != nullptr &&
                    _localWorkingMemory._hasEnemyFlag.value() == false &&
                    !nearEnemyF) {
                    prio = PriorityLevel::HIGH_PRIORITY;
                }
            } break;

            case WarSceneOrder::WarOrder::RECOVER_FLAG: {
                if (_globalWorkingMemory._flagCarriers[enemyTeamID].value() != nullptr &&
                    _globalWorkingMemory._flagRetrieverCount[ownTeamID].value() < 2) {
                    if (_localWorkingMemory._hasEnemyFlag.value() == false) {
                        prio = PriorityLevel::LOW_PRIORITY;
                    } else {
                        prio = PriorityLevel::MAX_PRIORITY;
                    }
                }
            } break;
        }
        priority[order->getID()] = to_uint(prio);
    }

    for (GOAPGoal& goal : goalList()) {
        goal.relevancy(priority[goal.getID()]);
    }
    GOAPGoal* goal = findRelevantGoal();
    
    if (goal != nullptr) {
        _planStatus = Util::stringFormat("Current goal: [ %s ]\n", goal->getName().c_str());

        // Hack to loop in idle
        worldState().setVariable(GOAPFact(Fact::IDLING), GOAPValue(false));
        worldState().setVariable(GOAPFact(Fact::NEAR_ENEMY_FLAG) , GOAPValue(nearEnemyF));

        if (replanGoal()) {
            _planStatus += Util::stringFormat(
                "The plan for goal [ %s ] involves [ %d ] actions.\n",
                goal->getName().c_str(),
                goal->getCurrentPlan().size());
            beginPlan(*goal);
        } else {
            _planStatus += Util::stringFormat("%s\n", getPlanLog().c_str());
            _planStatus += Util::stringFormat("Can't generate plan for goal [ %s ]\n",
                  goal->getName().c_str());
        }

        PRINT(_planStatus.c_str());
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type,
                                    const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr,
                  "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;
    _actionState[to_uint(type)] = true;

    bool atHome = atHomeBase();
    switch (type) {
        case ActionType::IDLE: {
            PRINT("Starting idle action");
            assert(atHome);
            _entity->stop();
        } break;
        case ActionType::APPROACH_ENEMY_FLAG: {
            PRINT("Starting approach flag action");
            _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value());
        } break;
        case ActionType::CAPTURE_ENEMY_FLAG: {
            PRINT("Starting capture flag action");
            assert(nearEnemyFlag());
        } break;
        case ActionType::SCORE_FLAG: {
            PRINT("Starting score action");
            if (_globalWorkingMemory._flagsAtBase[ownTeamID].value()) {
                invalidateCurrentPlan();
                return false;
            }
            assert(/*atHome && */_globalWorkingMemory._flagsAtBase[ownTeamID].value() == true);
        } break;
        case ActionType::RETURN_TO_BASE: {
            PRINT("Starting return to base action");
            _entity->updateDestination(_initialFlagPositions[ownTeamID]);
        } break;
        case ActionType::ATTACK_ENEMY: {
            PRINT("Starting attack enemy action");
            if (_localWorkingMemory._currentTarget.value() == nullptr &&
                _globalWorkingMemory._teamAliveCount[enemyTeamID].value() > 0) {

                if (_localWorkingMemory._isFlagRetriever.value() == false) {
                    SceneGraphNode* enemy = _visualSensor->getClosestNode(g_enemyTeamContainer);
                    _entity->updateDestination(enemy->getComponent<PhysicsComponent>()->getPosition(), true);
                    _localWorkingMemory._currentTarget.value(enemy);
                } else {
                    if (_globalWorkingMemory._flagCarriers[enemyTeamID].value() == nullptr) {
                        invalidateCurrentPlan();
                        return false;
                    }
                    SceneGraphNode* enemy =
                        _globalWorkingMemory._flagCarriers[enemyTeamID]
                            .value()
                            ->getUnitRef()
                            ->getBoundNode();
                    _entity->updateDestination(enemy->getComponent<PhysicsComponent>()->getPosition(), true);
                    _localWorkingMemory._currentTarget.value(enemy);
                }
            }
        } break;
        case ActionType::RECOVER_FLAG: {
            PRINT("Starting recover flag action");
            _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value());
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

    bool atHome = atHomeBase();
    switch (type) {
        case ActionType::IDLE: {
            PRINT("Idle action over");
        } break;
        case ActionType::APPROACH_ENEMY_FLAG: {
            PRINT("Approach flag action over");
            _entity->stop();
        } break;
        case ActionType::SCORE_FLAG: {
            PRINT("Score flag action over");
            assert(_localWorkingMemory._hasEnemyFlag.value());

            U8 score = _globalWorkingMemory._score[ownTeamID].value();
            _globalWorkingMemory._score[ownTeamID].value(score + 1);
            _localWorkingMemory._hasEnemyFlag.value(false);

            _globalWorkingMemory._flags[enemyTeamID].value()->setParent(
                GET_ACTIVE_SCENEGRAPH().getRoot());
            PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                                  .value()
                                  ->getComponent<PhysicsComponent>();
            pComp->popTransforms();

            for (const AITeam::TeamMap::value_type& member : currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::HAVE_SCORED, _entity);
            }

            dynamic_cast<WarScene*>(GET_ACTIVE_SCENE())->registerPoint(ownTeamID);

        } break;
        case ActionType::CAPTURE_ENEMY_FLAG: {
            PRINT("Capture flag action over");
            assert(!_localWorkingMemory._hasEnemyFlag.value());

            // Attach flag to entity
            {
                PhysicsComponent* pComp = _globalWorkingMemory._flags[enemyTeamID]
                    .value()->getComponent<PhysicsComponent>();
                vec3<F32> prevScale(pComp->getScale());
                vec3<F32> prevPosition(pComp->getPosition());

                SceneGraphNode* targetNode = _entity->getUnitRef()->getBoundNode();
                _globalWorkingMemory._flags[enemyTeamID].value()->setParent(*targetNode);
                pComp->pushTransforms();
                pComp->setPosition(vec3<F32>(0.0f, 0.75f, 1.5f));
                pComp->setScale(
                    (prevScale /
                    targetNode->getComponent<PhysicsComponent>()->getScale()) * vec3<F32>(1.0f, 2.0f, 1.0f));
            }

            _localWorkingMemory._hasEnemyFlag.value(true);
            _globalWorkingMemory._flagCarriers[ownTeamID].value(_entity);

            for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::HAVE_FLAG, _entity);
            }
            
            const AITeam* const enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
            for (const AITeam::TeamMap::value_type& enemy : enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, AIMsg::ENEMY_HAS_FLAG, _entity);
            }
        } break;
        case ActionType::RETURN_TO_BASE: {
            PRINT("Return to base action over");
            _entity->stop();
            assert(atHome);
        } break;
        case ActionType::ATTACK_ENEMY: {
            PRINT("Attack enemy action over");
            _localWorkingMemory._currentTarget.value(nullptr);
        } break;
        case ActionType::RECOVER_FLAG: {
            PRINT("Recover flag action over");
            for (const AITeam::TeamMap::value_type& member : currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::RETURNED_FLAG, _entity);
            }
        } break;
        default: {
            assert(false);
        } break;
    };

    GOAPAction::operationsIterator o;
    for (o = std::begin(warAction->effects()); o != std::end(warAction->effects()); ++o) {
        performActionStep(o);
    }

    PRINT("\n");
    _actionState[to_uint(type)] = false;
    return advanceGoal();
}

bool WarSceneAISceneImpl::checkCurrentActionComplete(const GOAPAction& planStep) {
    const WarSceneAction& warAction = static_cast<const WarSceneAction&>(planStep);
    ActionType type = warAction.actionType();
    if (!_actionState[to_uint(type)]) {
        return false;
    }

    const AITeam* const currentTeam = _entity->getTeam();
   
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    const SceneGraphNode& currentNode = *(_entity->getUnitRef()->getBoundNode());
    const SceneGraphNode& enemyFlag = *(_globalWorkingMemory._flags[enemyTeamID].value());

    bool state = false;
    bool atHome = atHomeBase();
    const BoundingBox& bb1 = currentNode.getBoundingBoxConst();
    switch (type) {
        case ActionType::APPROACH_ENEMY_FLAG:
        case ActionType::CAPTURE_ENEMY_FLAG: {
            state = nearEnemyFlag();
            if (!state) {
                _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[enemyTeamID].value(), true);
            }
        } break;

        case ActionType::SCORE_FLAG: {
            if (_localWorkingMemory._enemyHasFlag.value()) {
                invalidateCurrentPlan();
                return false;
            }

            state = atHome &&
                _globalWorkingMemory._flagsAtBase[ownTeamID].value() == true &&
                _entity->destinationReached();

            if (!state){
                _entity->updateDestination(_initialFlagPositions[ownTeamID]);
            }
          
        } break;

        case ActionType::IDLE:
        case ActionType::RETURN_TO_BASE: {
            state = atHome;
            if (!state && _entity->destinationReached()) {
                _entity->updateDestination(_initialFlagPositions[ownTeamID]);
            }
        } break;
        case ActionType::ATTACK_ENEMY: {
            SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
            if (!enemy) {
                state = true;
            } else {
                NPC* targetNPC = getUnitForNode(g_enemyTeamContainer, enemy)->getUnitRef();
                state = targetNPC->getAttribute(to_uint(UnitAttributes::ALIVE_FLAG)) == 0;
                if (!state) {
                    if (_entity->getPosition().distanceSquared(targetNPC->getPosition()) >= g_ATTACK_RADIUS_SQ) {
                        _entity->updateDestination(targetNPC->getPosition(), true);
                    }
                }
            }
        } break;
        case ActionType::RECOVER_FLAG: {
            state = _globalWorkingMemory._flagsAtBase[ownTeamID].value();
            if (!state) {
                _entity->updateDestination(_globalWorkingMemory._teamFlagPosition[ownTeamID].value());
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
        case AIMsg::RETURNED_FLAG:
        case AIMsg::HAVE_FLAG: {
            invalidateCurrentPlan();
        } break;
        case AIMsg::ENEMY_HAS_FLAG: {
            _localWorkingMemory._enemyHasFlag.value(true);
            worldState().setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(true));
        } break;
        case AIMsg::ATTACK: {
            if (!_localWorkingMemory._currentTarget.value() || 
                _localWorkingMemory._currentTarget.value()->getGUID() != 
                sender->getUnitRef()->getBoundNode()->getGUID())
            {
                invalidateCurrentPlan();
                _localWorkingMemory._currentTarget.value(sender->getUnitRef()->getBoundNode());
            }
        } break;

        case AIMsg::HAVE_DIED: {
            bool hadFlag = msg_content.constant_cast<bool>();
            if (hadFlag) {
                U8 senderTeamID = sender->getTeamID();
                U8 ownTeamID = _entity->getTeamID();
                if (ownTeamID == senderTeamID) {
                    worldState().setVariable(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));
                } else{
                    _localWorkingMemory._enemyHasFlag.value(false);
                    worldState().setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));
                }
                invalidateCurrentPlan();
            }

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

bool WarSceneAISceneImpl::atHomeBase() const {
    return _entity->getPosition().distanceSquared(
               _initialFlagPositions[_entity->getTeam()->getTeamID()]) <=
           g_ATTACK_RADIUS_SQ;
}

bool WarSceneAISceneImpl::nearOwnFlag() const {
    return _entity->getPosition().distanceSquared(
               _globalWorkingMemory
                   ._teamFlagPosition[_entity->getTeam()->getTeamID()]
                   .value()) <= g_ATTACK_RADIUS_SQ;
}

bool WarSceneAISceneImpl::nearEnemyFlag() const {
    return _entity->getPosition().distanceSquared(
               _globalWorkingMemory
                   ._teamFlagPosition[1 - _entity->getTeam()->getTeamID()]
                   .value()) <= g_ATTACK_RADIUS_SQ;
}

void WarSceneAISceneImpl::updatePositions() {
    _globalWorkingMemory._teamFlagPosition[0].value(
        _visualSensor->getNodePosition(
            g_flagContainer,
            _globalWorkingMemory._flags[0].value()->getGUID()));

    _globalWorkingMemory._teamFlagPosition[1].value(
        _visualSensor->getNodePosition(
            g_flagContainer,
            _globalWorkingMemory._flags[1].value()->getGUID()));

    _globalWorkingMemory._flagsAtBase[0].value(
        _globalWorkingMemory._teamFlagPosition[0].value().distanceSquared(
            _initialFlagPositions[0]) <= g_ATTACK_RADIUS_SQ);

    _globalWorkingMemory._flagsAtBase[1].value(
        _globalWorkingMemory._teamFlagPosition[1].value().distanceSquared(
            _initialFlagPositions[1]) <= g_ATTACK_RADIUS_SQ);

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));

    bool atHome = atHomeBase();
    U8 teamID = currentTeam->getTeamID();

    // If the enemy dropped the flag and we are near it, return it to base
    if (!_localWorkingMemory._enemyHasFlag.value()) {
        if (!_globalWorkingMemory._flagsAtBase[teamID].value()) {
            if (nearOwnFlag() && !atHome) {
                _globalWorkingMemory._flags[teamID].value()->setParent(
                    GET_ACTIVE_SCENEGRAPH().getRoot());
                PhysicsComponent* pComp =
                    _globalWorkingMemory._flags[teamID]
                        .value()
                        ->getComponent<PhysicsComponent>();
                pComp->popTransforms();
                _globalWorkingMemory._flagsAtBase[teamID].value(true);
            }
           for (const AITeam::TeamMap::value_type& member :
                 currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, AIMsg::RETURNED_FLAG, teamID);
            }
            for (const AITeam::TeamMap::value_type& enemy : enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, AIMsg::RETURNED_FLAG, teamID);
            }
        }
    }

    if (_type != AIType::HEAVY) {
        BoundingSphere boundingSphere(_entity->getUnitRef()->getBoundNode()->getBoundingSphereConst());
        const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();
        for (const AITeam::TeamMap::value_type& enemy : enemyMembers) {
            if (boundingSphere.Collision(enemy.second->getUnitRef()->getBoundNode()->getBoundingSphereConst())) {
                if (_localWorkingMemory._currentTarget.value() == nullptr ||
                    _localWorkingMemory._currentTarget.value()->getGUID() != enemy.second->getGUID()) {
                    invalidateCurrentPlan();
                    _localWorkingMemory._currentTarget.value(enemy.second->getUnitRef()->getBoundNode());
                }
            }
        }
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

    #if defined(PRINT_AI_TO_FILE)
        _WarAIOutputStream.flush();
    #endif
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    if (!AIManager::getInstance().getNavMesh(_entity->getAgentRadiusCategory())) {
        return;
    }

    if (_globalWorkingMemory._teamAliveCount[0].value() == 0) {
        dynamic_cast<WarScene*>(GET_ACTIVE_SCENE())->registerPoint(0);
        return;
    }

    if (_globalWorkingMemory._teamAliveCount[1].value() == 0) {
        dynamic_cast<WarScene*>(GET_ACTIVE_SCENE())->registerPoint(1);
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
                _entity->stop();
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

    assert(!goalList().empty());
    if (!getActiveGoal()) {
        requestOrders();
    } else {
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
        PRINT("\t\t Changing \"%s\" from \"%s\" to \"%s\"",
                         WarSceneFactName(crtFact),
                         GOAPValueName(oldVal),
                         GOAPValueName(newVal));
    }
    worldState().setVariable(crtFact, newVal);
    return true;
}

bool WarSceneAISceneImpl::printActionStats(const GOAPAction& planStep) const {
    PRINT("Action [ %s ]", planStep.name().c_str());
    return true;
}

void WarSceneAISceneImpl::printWorkingMemory() const {
    PRINT(toString().c_str());
}

stringImpl WarSceneAISceneImpl::toString() const {
    const AITeam* const currentTeam = _entity->getTeam();
    U8 ownTeamID = currentTeam->getTeamID();
    U8 enemyTeamID = 1 - ownTeamID;

    stringImpl ret(Util::stringFormat("Unit: [ %s ]\n", _entity->getName().c_str()));
    ret +="--------------- Working memory state BEGIN ----------------------------\n";
    ret += Util::stringFormat(
        "        Current position: - [ %4.1f , %4.1f, %4.1f]\n",
        _entity->getPosition().x, _entity->getPosition().y,
        _entity->getPosition().z);
    ret += Util::stringFormat(
        "        Flag Positions - OwnTeam : [ %4.1f , %4.1f, %4.1f] | Enemy Team : [ %4.1f , "
        "%4.1f, %4.1f]\n",
        _globalWorkingMemory._teamFlagPosition[ownTeamID].value().x,
        _globalWorkingMemory._teamFlagPosition[ownTeamID].value().y,
        _globalWorkingMemory._teamFlagPosition[ownTeamID].value().z,
        _globalWorkingMemory._teamFlagPosition[enemyTeamID].value().x,
        _globalWorkingMemory._teamFlagPosition[enemyTeamID].value().y,
        _globalWorkingMemory._teamFlagPosition[enemyTeamID].value().z);
    ret += Util::stringFormat(
        "        Flags at base - OwnTeam : [ %s ] | Enemy Team : [ %s ]\n",
        _globalWorkingMemory._flagsAtBase[ownTeamID].value() ? "true" : "false",
        _globalWorkingMemory._flagsAtBase[enemyTeamID].value() ? "true" : "false");
    ret += Util::stringFormat(
        "        Flags carriers - OwnTeam : [ %s ] | Enemy Team : [ %s ]\n",
        _globalWorkingMemory._flagCarriers[ownTeamID].value() ? _globalWorkingMemory._flagCarriers[ownTeamID].value()->getName().c_str() : "none",
        _globalWorkingMemory._flagCarriers[enemyTeamID].value() ? _globalWorkingMemory._flagCarriers[enemyTeamID].value()->getName().c_str() : "none");

    
    ret += Util::stringFormat(
        "        Score - OwnTeam : [ %d ] | Enemy Team : [ %d ]\n",
        _globalWorkingMemory._score[ownTeamID].value(),
        _globalWorkingMemory._score[enemyTeamID].value());
    ret += Util::stringFormat(
        "        Has enemy flag: [ %s ]\n",
        _localWorkingMemory._hasEnemyFlag.value() ? "true" : "false");
    ret += Util::stringFormat(
        "        Enemy has flag: [ %s ]\n",
        _localWorkingMemory._enemyHasFlag.value() ? "true" : "false");
    ret += Util::stringFormat(
        "        Is flag retriever: [ %s ]\n",
        _localWorkingMemory._isFlagRetriever.value() ? "true" : "false");
    ret += Util::stringFormat(
        "        Flag retriever count - Own Team: [ %d ] Enemy Team: [ %d]\n",
        _globalWorkingMemory._flagRetrieverCount[ownTeamID].value(),
        _globalWorkingMemory._flagRetrieverCount[enemyTeamID].value());
    ret += Util::stringFormat(
        "       Team count - Own Team: [ %d ] Enemy Team: [ %d]\n",
        _globalWorkingMemory._teamAliveCount[ownTeamID].value(),
        _globalWorkingMemory._teamAliveCount[enemyTeamID].value());
    for (std::pair<GOAPFact, GOAPValue> var : worldState().vars_) {
        ret += Util::stringFormat("        World state fact [ %s ] : [ %s ]\n",
                                  WarSceneFactName(var.first),
                                  var.second ? "true" : "false");
    }
    ret += "--------------- Working memory state END ----------------------------\n";

    if (getActiveGoal()) {
        const GOAPAction* activeAction = getActiveAction();
        if (activeAction) {
            ret += "Active action: " + activeAction->name() + "\n";
        }
    }

    SceneGraphNode* enemy = _localWorkingMemory._currentTarget.value();
    if (enemy != nullptr) {
        ret += "Active target: " + enemy->getName() + "\n";
    }

    ret += _planStatus;

    return ret;
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

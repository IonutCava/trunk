#include "Headers/WarSceneAISceneImpl.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
    namespace AI {

static const D32 g_ATTACK_RADIUS = 5;
static const U32 g_myTeamContainer = 0;
static const U32 g_enemyTeamContainer = 1;
static const U32 g_flagContainer = 2;

PositionFact     WorkingMemory::_team1FlagPosition;
PositionFact     WorkingMemory::_team2FlagPosition;
SmallCounterFact WorkingMemory::_team1Count;
SmallCounterFact WorkingMemory::_team2Count;
SGNNodeFact      WorkingMemory::_flags[2];
AINodeFact       WorkingMemory::_flagCarrier;
AINodeFact       WorkingMemory::_enemyFlagCarrier;

vec3<F32> WarSceneAISceneImpl::_initialFlagPositions[2];

WarSceneAISceneImpl::WarSceneAISceneImpl() : AISceneImpl(),
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
    for(GOAPAction*& action : actionSetPtr()) {
        SAFE_DELETE(action);
    }
    actionSetPtr().clear();
}

void WarSceneAISceneImpl::registerAction(GOAPAction* const action) {
    WarSceneAction* const warAction = dynamic_cast<WarSceneAction*>(action);
    warAction->setParentAIScene(this);
    switch(warAction->actionType()){
        case ACTION_APPROACH_FLAG : AISceneImpl::registerAction(New ApproachFlag(*warAction)); return;
        case ACTION_CAPTURE_FLAG  : AISceneImpl::registerAction(New CaptureFlag(*warAction));  return;
        case ACTION_RETURN_FLAG   : AISceneImpl::registerAction(New ReturnFlag(*warAction));   return;
        case ACTION_RECOVER_FLAG  : AISceneImpl::registerAction(New RecoverFlag(*warAction));  return;
        case ACTION_PROTECT_FLAG_CARRIER : AISceneImpl::registerAction(New ProtectFlagCarrier(*warAction)); return;
    };

    AISceneImpl::registerAction(New WarSceneAction(*warAction));
}

void WarSceneAISceneImpl::init() {

    _visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    //_audioSensor = dynamic_cast<AudioSensor*>(_entity->getSensor(AUDIO_SENSOR));
    DIVIDE_ASSERT(_visualSensor != nullptr, "WarSceneAISceneImpl error: No visual sensor found for current AI entity!");
    //DIVIDE_ASSERT(_audioSensor != nullptr, "WarSceneAISceneImpl error: No audio sensor found for current AI entity!");

    AITeam* currentTeam = _entity->getTeam();
    AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
    const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
    const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();

    FOR_EACH(const AITeam::TeamMap::value_type& member, teamAgents) {
        _visualSensor->followSceneGraphNode(g_myTeamContainer, member.second->getUnitRef()->getBoundNode());
    }

    FOR_EACH(const AITeam::TeamMap::value_type& enemy, enemyMembers) {
        _visualSensor->followSceneGraphNode(g_enemyTeamContainer, enemy.second->getUnitRef()->getBoundNode());
    }
        
    _visualSensor->followSceneGraphNode(g_flagContainer, WorkingMemory::_flags[0].value());
    _visualSensor->followSceneGraphNode(g_flagContainer, WorkingMemory::_flags[1].value());

    _initialFlagPositions[0].set(WorkingMemory::_flags[0].value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
    _initialFlagPositions[1].set(WorkingMemory::_flags[1].value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
}

void WarSceneAISceneImpl::requestOrders() {
    const vectorImpl<Order* >& orders = _entity->getTeam()->requestOrders();
    U8 priority[WarSceneOrder::ORDER_PLACEHOLDER];
    memset(priority, 0, sizeof(U8) * WarSceneOrder::ORDER_PLACEHOLDER);

    resetActiveGoals();

    for (Order* const order : orders) {
        WarSceneOrder::WarOrder orderId = static_cast<WarSceneOrder::WarOrder>(dynamic_cast<WarSceneOrder*>(order)->getId());
        switch (orderId) {
            case WarSceneOrder::ORDER_FIND_ENEMY_FLAG : {
                if (!_workingMemory._hasEnemyFlag.value() && !_workingMemory._enemyFlagNear.value()) {
                    priority[orderId] = _workingMemory._teamMateHasFlag.value() ? 0 : 128;
                }
            } break;
            case WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG : {
                if (_workingMemory._enemyFlagNear.value() && !_workingMemory._teamMateHasFlag.value()) {
                    priority[orderId] =  128;
                }
            } break;
            case WarSceneOrder::ORDER_RETURN_ENEMY_FLAG : {
                if (_workingMemory._hasEnemyFlag.value()) {
                    priority[orderId] = 255;
                }
            } break;
            case WarSceneOrder::ORDER_PROTECT_FLAG_CARRIER : {
                if (!_workingMemory._hasEnemyFlag.value() && _workingMemory._teamMateHasFlag.value()) {
                    priority[orderId] = 128;
                    if (_workingMemory._flagCarrierNear.value()) {
                        priority[orderId] += 64;
                    }
                }
            } break;
            case WarSceneOrder::ORDER_RETRIEVE_FLAG : {
                if (_workingMemory._enemyHasFlag.value() && !_workingMemory._hasEnemyFlag.value()) {
                    priority[orderId] = 128;
                    if (_workingMemory._enemyFlagCarrierNear.value()) {
                        priority[orderId]++;
                    }
                }
            } break;
        } 
    }

    for (GOAPGoal& goal : goalList()) {
        if (goal.getName().compare("Find enemy flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_FIND_ENEMY_FLAG]);
        }
        if (goal.getName().compare("Capture enemy flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG]);
        }
        if (goal.getName().compare("Return enemy flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_RETURN_ENEMY_FLAG]);
        }
        if (goal.getName().compare("Protect flag carrier") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_PROTECT_FLAG_CARRIER]);
        }
        if (goal.getName().compare("Retrieve flag") == 0) {
            goal.relevancy(priority[WarSceneOrder::ORDER_RETRIEVE_FLAG]);
        }
    }

    if (findRelevantGoal() != nullptr && replanGoal()) {
        D_PRINT_FN("Current goal: [ %s ]", getActiveGoal()->getName().c_str());
        D_PRINT_FN("The Plan for [ %d ] involves [ %d ] actions.", _entity->getGUID(), getActiveGoal()->getCurrentPlan().size());
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type, const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (type) {
        case ACTION_APPROACH_FLAG : {
            D_PRINT_FN("Starting approach flag action");
            _entity->updateDestination(currentTeam->getTeamID() == 0 ? _workingMemory._team2FlagPosition.value() : 
                                                                       _workingMemory._team1FlagPosition.value());
        } break;
        case ACTION_CAPTURE_FLAG : {
            D_PRINT_FN("Starting capture flag action");
        } break;
        case ACTION_RETURN_FLAG : {
            D_PRINT_FN("Starting return flag action");
            _entity->updateDestination(currentTeam->getTeamID() == 1 ? _workingMemory._team2FlagPosition.value() :
                                                                       _workingMemory._team1FlagPosition.value());
        } break;
        default : {
            D_PRINT_FN("Starting normal action");
        } break;
    };
        
    return true;
}

bool WarSceneAISceneImpl::postAction(ActionType type, const WarSceneAction* warAction) {
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (type) {
        case ACTION_APPROACH_FLAG : {
            D_PRINT_FN("Approach flag action over");
            _workingMemory._enemyFlagNear.value(true);
        } break;
        case ACTION_CAPTURE_FLAG : {
            D_PRINT_FN("Capture flag action over");

            if (_workingMemory._teamMateHasFlag.value()) {
                invalidateCurrentPlan();
                return false; 
            }

            _workingMemory._hasEnemyFlag.value(true);
            FOR_EACH(const AITeam::TeamMap::value_type& member, currentTeam->getTeamMembers()) {
                _entity->sendMessage(member.second, HAVE_FLAG, _entity);
            }
            const AITeam* const enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
            FOR_EACH(const AITeam::TeamMap::value_type& enemy, enemyTeam->getTeamMembers()) {
                _entity->sendMessage(enemy.second, ENEMY_HAS_FLAG, _entity);
            }

            U8 flag = 1 - currentTeam->getTeamID();
            PhysicsComponent* pComp = _workingMemory._flags[flag].value()->getComponent<PhysicsComponent>();
            vec3<F32> prevScale(pComp->getConstTransform()->getScale());

            SceneGraphNode* targetNode = _entity->getUnitRef()->getBoundNode();
            _workingMemory._flags[flag].value()->attachToNode(targetNode);
            pComp->setPosition(vec3<F32>(0.0f, 0.75f, 1.5f));
            pComp->setScale(prevScale / targetNode->getComponent<PhysicsComponent>()->getConstTransform()->getScale());
        } break;

        case ACTION_RETURN_FLAG : {
            PRINT_FN("Return flag action over");

            U8 flag = 1 - currentTeam->getTeamID();
            PhysicsComponent* pComp = _workingMemory._flags[flag].value()->getComponent<PhysicsComponent>();
            vec3<F32> prevScale(pComp->getConstTransform()->getScale());

            SceneGraphNode* targetNode = _workingMemory._flags[flag].value()->getParent();
            _workingMemory._flags[flag].value()->attachToRoot();
            pComp->setPosition(_initialFlagPositions[flag]);
            pComp->setScale(prevScale * targetNode->getComponent<PhysicsComponent>()->getConstTransform()->getScale());
        } break;

        default : {
            PRINT_FN("Normal action over");
        } break;
    };
    PRINT_FN("   %s ", warAction->name().c_str());
    for(GOAPAction::operationsIterator o = warAction->effects().begin(); o != warAction->effects().end(); o++) {
        performActionStep(o);
    }
    PRINT_F("\n");
    return advanceGoal();
}

bool WarSceneAISceneImpl::checkCurrentActionComplete(const GOAPAction* planStep) {
    assert(planStep != nullptr);
    const WarSceneAction& warAction = *dynamic_cast<const WarSceneAction*>(planStep);
    const AITeam* const currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");
    const SceneGraphNode* const currentNode = _entity->getUnitRef()->getBoundNode();
    const SceneGraphNode* const enemyFlag = _workingMemory._flags[1 - currentTeam->getTeamID()].value();

    bool state = false;
    const BoundingBox& bb1 = currentNode->getBoundingBoxConst();
    switch (warAction.actionType()) {
        case ACTION_APPROACH_FLAG : {
            state = enemyFlag->getBoundingBoxConst().Collision(bb1);
        } break;
        case ACTION_CAPTURE_FLAG : {
            state = enemyFlag->getBoundingBoxConst().Collision(bb1);
        } break;
        case ACTION_RETURN_FLAG : {
            const SceneGraphNode* const ownFlag = _workingMemory._flags[currentTeam->getTeamID()].value();
            const BoundingBox& ownFlagBB = ownFlag->getBoundingBoxConst();
            // Our flag is in its original position and the 2 flags are touching
            state = (enemyFlag->getBoundingBoxConst().Collision(ownFlagBB) && 
                    ownFlag->getComponent<PhysicsComponent>()->getConstTransform()->getPosition().distanceSquared(_initialFlagPositions[currentTeam->getTeamID()]) < g_ATTACK_RADIUS);
        } break;
        default : {
            PRINT_FN("Normal action over");
        } break;
    };
    if (state) {
        return warAction.postAction();
    }
    return state;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content) {
    switch(msg){
        case HAVE_FLAG : {
            _workingMemory._teamMateHasFlag.value(true);
            _workingMemory._flagCarrier.value(sender);
        } break;
        case ENEMY_HAS_FLAG: {
            _workingMemory._enemyHasFlag.value(true);
            _workingMemory._enemyFlagCarrier.value(sender);
        } break;
    };

    if (msg == HAVE_FLAG || msg == ENEMY_HAS_FLAG) {
        invalidateCurrentPlan();
    }
}

void WarSceneAISceneImpl::updatePositions() {
    if (_workingMemory._flagCarrier.value()) {
        F32 distanceSq = _visualSensor->getDistanceToNodeSq(g_myTeamContainer, _workingMemory._flagCarrier.value()->getUnitRef()->getBoundNode()->getGUID());
        bool newState = (distanceSq < g_ATTACK_RADIUS * g_ATTACK_RADIUS);
        if (newState != _workingMemory._flagCarrierNear.value()) {
            invalidateCurrentPlan();
        }
        _workingMemory._flagCarrierNear.value(newState);
    }
    if (_workingMemory._enemyFlagCarrier.value()) {
        F32 distanceSq = _visualSensor->getDistanceToNodeSq(g_enemyTeamContainer, _workingMemory._enemyFlagCarrier.value()->getUnitRef()->getBoundNode()->getGUID());
        bool newState = (distanceSq < g_ATTACK_RADIUS * g_ATTACK_RADIUS);
        if (newState != _workingMemory._enemyFlagCarrierNear.value()) {
            invalidateCurrentPlan();
        }
        _workingMemory._enemyFlagCarrierNear.value(newState);
    }

    if (_workingMemory._flags[0].value() != nullptr) {
        _workingMemory._team1FlagPosition.value(_visualSensor->getNodePosition(g_flagContainer, _workingMemory._flags[0].value()->getGUID()));
    }
    if (_workingMemory._flags[1].value() != nullptr) {
        _workingMemory._team2FlagPosition.value(_visualSensor->getNodePosition(g_flagContainer, _workingMemory._flags[1].value()->getGUID()));
    }

    if (_workingMemory._currentTargetEntity.value()) {
        _workingMemory._currentTargetPosition.value( _workingMemory._currentTargetEntity.value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
    }
}

void WarSceneAISceneImpl::processInput(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    _deltaTime += deltaTime;
    
    if (_deltaTime > getUsToSec(1.5)) {//wait 1 and a half seconds at the destination
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
    _visualSensorUpdateCounter = (_visualSensorUpdateCounter + 1) % (visualSensorUpdateFreq + 1);
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
            _workingMemory._team1Count.value(static_cast<U8>(team1Members.size()));
        }
        AITeam* team2 = AIManager::getInstance().getTeamByID(1);
        if (team2 != nullptr) {
            const AITeam::TeamMap& team2Members = team2->getTeamMembers();
            _workingMemory._team2Count.value(static_cast<U8>(team2Members.size()));
        }
        _workingMemory._staticDataUpdated = true;
    }
    
    if (!_workingMemory._currentTargetEntity.value()) {
        _workingMemory._currentTargetEntity.value(_visualSensor->getClosestNode(g_enemyTeamContainer));
    } 
}

bool WarSceneAISceneImpl::performAction(const GOAPAction* planStep) {
    if(planStep == nullptr) {
        return false;
    }
    return dynamic_cast<const WarSceneAction*>(planStep)->preAction();
}

bool WarSceneAISceneImpl::performActionStep(GOAPAction::operationsIterator step) {
    GOAPFact crtFact = step->first;
    GOAPValue newVal = step->second;
    GOAPValue oldVal = worldState().getVariable(crtFact);
    if (oldVal != newVal) {
        switch(static_cast<Fact>(crtFact)) {
            case EnemyVisible : {
            } break;
            case EnemyInAttackRange : {
            } break;
            case EnemyDead : {
            } break;
            case WaitingIdle : {
            } break;
            case AtTargetNode : {
            } break;
            case HasTargetNode : {
            } break;
        };
        PRINT_FN("\t\tChanging \"%s\" from \"%s\" to \"%s\"", WarSceneFactName(crtFact), GOAPValueName(oldVal), GOAPValueName(newVal));
    }
    worldState().setVariable(crtFact, newVal);
    return true;
}

    }; //namespace AI
}; //namespace Divide
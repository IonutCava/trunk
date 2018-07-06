#include "Headers/WarSceneAISceneImpl.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
using namespace AI;

static const D32 ATTACK_RADIUS = 5;

PositionFact     WorkingMemory::_team1FlagPosition;
PositionFact     WorkingMemory::_team2FlagPosition;
SmallCounterFact WorkingMemory::_team1Count;
SmallCounterFact WorkingMemory::_team2Count;
SGNNodeFact      WorkingMemory::_flags[2];

WarSceneAISceneImpl::WarSceneAISceneImpl() : AISceneImpl(),
                                            _tickCount(0),
                                            _orderRequestTryCount(0),
                                            _visualSensorUpdateCounter(0),
                                            _deltaTime(0ULL)
{
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
    for(GOAPAction*& action : actionSetPtr()){
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
    };

    AISceneImpl::registerAction(New WarSceneAction(*warAction));
}

static const U32 myTeamContainer = 0;
static const U32 enemyTeamContainer = 1;
static const U32 flagContainer = 2;

void WarSceneAISceneImpl::init() {
    VisualSensor* sensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    if (sensor) {
        AITeam* currentTeam = _entity->getTeam();
        const AITeam::TeamMap& teamAgents = currentTeam->getTeamMembers();
        FOR_EACH(const AITeam::TeamMap::value_type& member, teamAgents) {
            sensor->followSceneGraphNode(myTeamContainer, member.second->getUnitRef()->getBoundNode());
        }

        AITeam* enemyTeam = AIManager::getInstance().getTeamByID(currentTeam->getEnemyTeamID(0));
        if (enemyTeam != nullptr) {
            const AITeam::TeamMap& enemyMembers = enemyTeam->getTeamMembers();
            FOR_EACH(const AITeam::TeamMap::value_type& enemy, enemyMembers) {
                sensor->followSceneGraphNode(enemyTeamContainer, enemy.second->getUnitRef()->getBoundNode());
            }
        }
        sensor->followSceneGraphNode(flagContainer, WorkingMemory::_flags[0].value());
        sensor->followSceneGraphNode(flagContainer, WorkingMemory::_flags[1].value());
    }
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
                if (!_workingMemory._hasEnemyFlag.value()) {
                    priority[orderId] = _workingMemory._teamMateHasFlag.value() ? 0 : 128;

                    if (!_workingMemory._enemyHasFlag.value()) {
                        priority[orderId]++;
                    }
                    if (!_workingMemory._enemyFlagNear.value()) {
                        priority[orderId]++;
                    }
                }
            } break;
            case WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG : {
                if (_workingMemory._enemyFlagNear.value()) {
                    priority[orderId] = _workingMemory._teamMateHasFlag.value() ? 0 : 128;
                    if (!_workingMemory._hasEnemyFlag.value()) {
                        priority[orderId]++;
                    }
                    if (_workingMemory._enemyHasFlag.value()) {
                        priority[orderId]++;
                    }
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
                        priority[orderId]++;
                    }
                }
            } break;
            case WarSceneOrder::ORDER_RETRIEVE_FLAG : {
                if (_workingMemory._enemyHasFlag.value()) {
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
    AITeam* currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (type) {
        case ACTION_APPROACH_FLAG : {
            PRINT_FN("Starting approach flag action");
            _entity->updateDestination(currentTeam->getTeamID() == 0 ?  _workingMemory._team2FlagPosition.value() : _workingMemory._team1FlagPosition.value());
        } break;
        case ACTION_CAPTURE_FLAG : {
            PRINT_FN("Starting capture flag action");
            const BoundingBox& bb1 = _entity->getUnitRef()->getBoundNode()->getBoundingBoxConst();
            const BoundingBox& bb2 = _workingMemory._flags[currentTeam->getTeamID()].value()->getBoundingBoxConst();
            if (bb1.Collision(bb2)) {
                _workingMemory._hasEnemyFlag.value(true);
                _workingMemory._hasEnemyFlag.belief(1.0);
                FOR_EACH(const AITeam::TeamMap::value_type& member, currentTeam->getTeamMembers()) {
                    if(_entity->getGUID() != member.second->getGUID()) {
                        _entity->sendMessage(member.second, HAVE_FLAG, _entity);
                    }
                }
            }
        } break;
        case ACTION_RETURN_FLAG : {
            PRINT_FN("Starting return flag action");
        } break;
        default : {
            PRINT_FN("Starting normal action");
        } break;
    };
        
    return true;
}

bool WarSceneAISceneImpl::postAction(ActionType type, const WarSceneAction* warAction) {
    switch (type) {
        case ACTION_APPROACH_FLAG : {
            PRINT_FN("Approach flag action over");
        } break;
        case ACTION_CAPTURE_FLAG : {
            PRINT_FN("Capture flag action over");
        } break;
        case ACTION_RETURN_FLAG : {
            PRINT_FN("Return flag action over");
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

    bool state = false;
    AITeam* currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    switch (warAction.actionType()) {
        case ACTION_APPROACH_FLAG : {
            const BoundingBox& bb1 = _entity->getUnitRef()->getBoundNode()->getBoundingBoxConst();
            const BoundingBox& bb2 = _workingMemory._flags[1 - currentTeam->getTeamID()].value()->getBoundingBoxConst();
            state = bb1.Collision(bb2);
        } break;
        case ACTION_CAPTURE_FLAG : {
            PRINT_FN("Capture flag action over");
        } break;
        case ACTION_RETURN_FLAG : {
            PRINT_FN("Return flag action over");
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

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content){
    switch(msg){
        case HAVE_FLAG : {
            _workingMemory._teamMateHasFlag.value(true);
            _workingMemory._teamMateHasFlag.belief(1.0f);
            invalidateCurrentPlan();
        } break;
    };
}

void WarSceneAISceneImpl::updatePositions(){
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
    updatePositions();
}

void WarSceneAISceneImpl::update(const U64 deltaTime, NPC* unitRef){
    if (!unitRef) {
        return;
    }
    U8 visualSensorUpdateFreq = 10;
    _visualSensorUpdateCounter = (_visualSensorUpdateCounter + 1) % (visualSensorUpdateFreq + 1);
    /// Update sensor information
    VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    if (visualSensor && _visualSensorUpdateCounter == visualSensorUpdateFreq) {
        visualSensor->update(deltaTime);
    }
    AudioSensor* audioSensor = dynamic_cast<AudioSensor*>(_entity->getSensor(AUDIO_SENSOR));
    if (audioSensor) {
        audioSensor->update(deltaTime);
    }

    
    if (!_workingMemory._staticDataUpdated) {
        AITeam* team1 = AIManager::getInstance().getTeamByID(0);
        if (team1 != nullptr) {
            const AITeam::TeamMap& team1Members = team1->getTeamMembers();
            _workingMemory._team1Count.value(static_cast<U8>(team1Members.size()));
            _workingMemory._team1Count.belief(1.0f);
        }
        AITeam* team2 = AIManager::getInstance().getTeamByID(1);
        if (team2 != nullptr) {
            const AITeam::TeamMap& team2Members = team2->getTeamMembers();
            _workingMemory._team2Count.value(static_cast<U8>(team2Members.size()));
            _workingMemory._team2Count.belief(1.0f);
        }
        if (_workingMemory._flags[0].value() != nullptr) {
            _workingMemory._team1FlagPosition.value(_workingMemory._flags[0].value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
            _workingMemory._team1FlagPosition.belief(1.0f);
        }
        if (_workingMemory._flags[1].value() != nullptr) {
            _workingMemory._team2FlagPosition.value(_workingMemory._flags[1].value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
            _workingMemory._team2FlagPosition.belief(1.0f);
        }
        _workingMemory._staticDataUpdated = true;
    }
    
    if (!_workingMemory._currentTargetEntity.value()) {
        _workingMemory._currentTargetEntity.value(visualSensor->getClosestNode(enemyTeamContainer));
        _workingMemory._currentTargetEntity.belief(1.0f);
    } else {
        _workingMemory._currentTargetPosition.value( _workingMemory._currentTargetEntity.value()->getComponent<PhysicsComponent>()->getConstTransform()->getPosition());
        _workingMemory._currentTargetPosition.belief(1.0f);
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

};
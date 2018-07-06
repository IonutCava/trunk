#include "Headers/WarSceneAISceneImpl.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "AI/ActionInterface/Headers/AITeam.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

using namespace AI;

static const D32 ATTACK_RADIUS = 5;

PositionFact     WorkingMemory::_team1FlagPosition;
PositionFact     WorkingMemory::_team2FlagPosition;
SmallCounterFact WorkingMemory::_team1Count;
SmallCounterFact WorkingMemory::_team2Count;
SGNNodeFact      WorkingMemory::_flags[2];

WarSceneAISceneImpl::WarSceneAISceneImpl() : AISceneImpl(),
                                            _tickCount(0),
                                            _newPlan(false),
                                            _newPlanSuccess(false),
                                            _activeGoal(nullptr),
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

    clearActiveGoals();
    for (GOAPGoal& goal : goalList()) {
        goal.relevancy(0.0f);
        activateGoal(goal.getName());
    }

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
    if ((_activeGoal = findRelevantGoal()) != nullptr) {
        PRINT_FN("Current goal: [ %s ]", _activeGoal->getName().c_str());
        if (_activeGoal->plan(worldState(), actionSet())) {
            popActiveGoal(_activeGoal);
            _newPlan = true;
        }
    }
}

bool WarSceneAISceneImpl::preAction(ActionType type) {
    switch (type) {
        case ACTION_APPROACH_FLAG : {
            PRINT_FN("Starting approach flag action");
        } break;
        case ACTION_CAPTURE_FLAG : {
            PRINT_FN("Starting capture flag action");
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

bool WarSceneAISceneImpl::postAction(ActionType type) {
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
        
    return true;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content){
}

void WarSceneAISceneImpl::updatePositions(){
}

void WarSceneAISceneImpl::processInput(const U64 deltaTime) {
    if (!_entity) {
        return;
    }
    AIManager& aiMgr = AIManager::getInstance();
    if (!aiMgr.getNavMesh(_entity->getAgentRadiusCategory())) {
        return;
    }
    _deltaTime += deltaTime;

    AITeam* currentTeam = _entity->getTeam();
    DIVIDE_ASSERT(currentTeam != nullptr, "WarScene error: INVALID TEAM FOR INPUT UPDATE");

    if (_entity->destinationReached() && _workingMemory._staticDataUpdated) {
        if (_deltaTime > getUsToSec(1.5)) {//wait 1 and a half seconds at the destination

            const BoundingBox& bb1 = _entity->getUnitRef()->getBoundNode()->getBoundingBoxConst();
            const BoundingBox& bb2 = _workingMemory._flags[currentTeam->getTeamID()].value()->getBoundingBoxConst();

            if (bb1.Collision(bb2)) {

                _entity->stop();
                _entity->getUnitRef()->lookAt(currentTeam->getTeamID() == 0 ? _workingMemory._team2FlagPosition.value() : _workingMemory._team1FlagPosition.value());

            } else {

                if (currentTeam->getTeamID() == 0) { 
                    _entity->updateDestination(  _workingMemory._team2FlagPosition.value() );
                } else {
                    _entity->updateDestination( _workingMemory._team1FlagPosition.value() );
                }    

            }
            _deltaTime = 0;
        }
    }
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }

    if (_newPlan) {
        assert(_activeGoal != nullptr);
        handlePlan(_activeGoal->getCurrentPlan());
        _newPlan = false;
    }

    if (!_activeGoal && !goalList().empty()) {
        requestOrders();
    }

    updatePositions();
}

void WarSceneAISceneImpl::update(const U64 deltaTime, NPC* unitRef){
    if (!unitRef) {
        return;
    }

    updatePositions();

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

void WarSceneAISceneImpl::handlePlan(const GOAPPlan& plan) {
   GOAPPlan::const_iterator it;
   PRINT_FN("The Plan for [ %d ] involves [ %d ] actions.", _entity->getGUID(), plan.size());
   for (it = plan.begin(); it != plan.end(); it++) {
      performAction(*it);
   }
}

bool WarSceneAISceneImpl::performAction(const GOAPAction* planStep) {
    const WarSceneAction* warSceneAction = dynamic_cast<const WarSceneAction*>(planStep);
    assert(warSceneAction != nullptr);

    if (!warSceneAction->preAction()){
        return false;
    }
    PRINT_FN("   %s ", warSceneAction->name().c_str());
    
    GOAPValue oldVal = false;
    GOAPValue newVal = false;
    GOAPFact crtFact;
    for(GOAPAction::operationsIterator o = warSceneAction->effects().begin(); o != warSceneAction->effects().end(); o++) {
        crtFact = o->first;
        newVal  = o->second;
        oldVal  = worldState().getVariable(crtFact);
        if (oldVal != newVal) {
            PRINT_FN("\t\tChanging \"%s\" from \"%s\" to \"%s\"", WarSceneFactName(crtFact), GOAPValueName(oldVal), GOAPValueName(newVal));
            worldState().setVariable(crtFact, newVal);
        }
    }
    PRINT_F("\n");
    return warSceneAction->postAction();
}

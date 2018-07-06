#include "Headers/WarSceneAISceneImpl.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

using namespace AI;

static const D32 ATTACK_RADIUS = 4 * 4;

WarSceneAISceneImpl::WarSceneAISceneImpl() : AISceneImpl(),
                                            _tickCount(0),
                                            _newPlan(false),
                                            _newPlanSuccess(false),
                                            _orderReceived(false),
                                            _activeGoal(nullptr),
                                            _currentEnemyTarget(nullptr),
                                            _deltaTime(0ULL),
                                            _indexInMap(-1)
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
        case ACTION_WAIT     : AISceneImpl::registerAction(New WaitAction(*warAction));     return;
        case ACTION_SCOUT    : AISceneImpl::registerAction(New ScoutAction(*warAction));    return;
        case ACTION_APPROACH : AISceneImpl::registerAction(New ApproachAction(*warAction)); return;
        case ACTION_TARGET   : AISceneImpl::registerAction(New TargetAction(*warAction));   return;
        case ACTION_ATTACK   : AISceneImpl::registerAction(New AttackAction(*warAction));   return;
        case ACTION_RETREAT  : AISceneImpl::registerAction(New RetreatAction(*warAction));  return;
        case ACTION_KILL     : AISceneImpl::registerAction(New KillAction(*warAction));     return;
    };

    AISceneImpl::registerAction(New WarSceneAction(*warAction));
}

void WarSceneAISceneImpl::registerGoal(const GOAPGoal& goal) {
    AISceneImpl::registerGoal(WarSceneGoal(goal));
}

void WarSceneAISceneImpl::receiveOrder(AI::Order order) {
    switch(order){
        case ORDER_FIND_ENEMY : {
            activateGoal("Find Enemy");
        }; break;
        case ORDER_KILL_ENEMY : {
            activateGoal("Attack Enemy");
        }; break;
        case ORDER_WAIT : {
            activateGoal("Wait");
        }; break;
    };
    _orderReceived = true;
}

void WarSceneAISceneImpl::processMessage(AIEntity* sender, AIMsg msg, const cdiggins::any& msg_content){
}

void WarSceneAISceneImpl::updatePositions(){
}

static U8 stage = 0;
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

    if (_entity->destinationReached()) {
        I64 currentIndex = 0;
        
        bool foundId = (_indexInMap != -1);
        if (_indexInMap == -1) {
            _indexInMap = 0;
        }

        const AITeam::teamMap& teamAgents = currentTeam->getTeamMembers();
        FOR_EACH(const AITeam::teamMap::value_type& member, teamAgents) {
            if (_entity->getGUID() != member.second->getGUID()) {
                _entity->sendMessage(member.second, CHANGE_DESTINATION_POINT, 0);
                if (!foundId) {
                    _indexInMap++;
                }
            } else {
                currentIndex = member.first;
                foundId = true;
            }
        }
        
        if (currentTeam->getTeamID() == 1 && _deltaTime > getUsToSec(1.5)) { //wait 1 and a half seconds at the destination
            // team 1 moves randomly. team2 chases team 1
            _entity->updateDestination( aiMgr.getNavMesh(_entity->getAgentRadiusCategory())->getRandomPosition() );
            _deltaTime = 0;
        }
    }
     
    if (!_currentEnemyTarget) {
        AITeam* enemyTeam = aiMgr.getTeamByID(currentTeam->getEnemyTeamID(0));
        if (enemyTeam != nullptr) {
            const AITeam::teamMap& enemyMembers = enemyTeam->getTeamMembers();

            I32 i = 0; 
            AIEntity* selectedEnemy = nullptr;
            FOR_EACH(const AITeam::teamMap::value_type& enemy, enemyMembers) {
                selectedEnemy = enemy.second;
                if (i++ >= _indexInMap) {
                    break;
                }
            }
            _currentEnemyTarget = selectedEnemy;
        }
    }

    if (currentTeam->getTeamID() == 2 && _deltaTime > getUsToMs(250)) {
        AITeam* enemyTeam = aiMgr.getTeamByID(currentTeam->getEnemyTeamID(0));
        if (enemyTeam != nullptr) {
            AITeam::teamMap::const_iterator it = enemyTeam->getTeamMembers().begin();
            I32 i = 0; 
            while (i < _indexInMap) {
                ++i;
                ++it;
            }
            _entity->updateDestination(_currentEnemyTarget->getPosition());
            _deltaTime = 0;
        }
    }

    if (_currentEnemyTarget && _currentEnemyTarget->getUnitRef()) {
        if (_entity->getPosition().distanceSquared(_currentEnemyTarget->getPosition()) < ATTACK_RADIUS) {
            if (_currentEnemyTarget->isMoving()) {
                const BoundingBox& bb1 = _entity->getUnitRef()->getBoundNode()->getBoundingBox();
                const BoundingBox& bb2 = _currentEnemyTarget->getUnitRef()->getBoundNode()->getBoundingBox();

                if (bb1.Collision(bb2)) {
                    _entity->moveBackwards();
                    _currentEnemyTarget->moveBackwards();
                }

                _currentEnemyTarget->stop();
                _entity->stop();

                _currentEnemyTarget->getUnitRef()->lookAt(_entity->getPosition());
                _entity->getUnitRef()->lookAt(_currentEnemyTarget->getPosition());
            }
        }
    }

    if (stage == 0) {
        receiveOrder(ORDER_FIND_ENEMY);
        stage = 1;
    }
    if (stage == 2) {
        receiveOrder(ORDER_KILL_ENEMY);
        stage = 3;
    }
    if (stage == 4) {
        receiveOrder(ORDER_WAIT);
        stage = 5;
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

    if (_orderReceived) {
        if ((_activeGoal = findRelevantGoal()) != nullptr) {
           PRINT_FN("Current goal: [ %s ]", _activeGoal->getName().c_str());
           if (_activeGoal->plan(worldState(), actionSet())) {
                popActiveGoal(_activeGoal);
                _newPlan = true;
           }
        }
        _orderReceived = false;
    }

    updatePositions();
}

void WarSceneAISceneImpl::update(NPC* unitRef){
    if (!unitRef) {
        return;
    }

    updatePositions();

    /// Update sensor information
    Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);
    if (visualSensor) {
        visualSensor->updatePosition(unitRef->getPosition());
    }
}

void WarSceneAISceneImpl::handlePlan(const GOAPPlan& plan) {
   GOAPPlan::const_iterator it;
   PRINT_FN("The Plan for [ %d ] involves [ %d ] actions.", _entity->getGUID(), plan.size());
   for (it = plan.begin(); it != plan.end(); it++) {
      performAction(*it);
   }
   stage += 1;
   stage = stage % 6;
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

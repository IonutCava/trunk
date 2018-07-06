#include "Headers/WarSceneAISceneImpl.h"
#include "AESOPActions/Headers/AESOPActions.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

using namespace AI;

static const D32 ATTACK_RADIUS = 4 * 4;

void printPlan(Aesop::Context &ctx, const Aesop::Plan &plan) {
   Aesop::Plan::const_iterator it;
   ctx.logEvent("The Plan:\n");
   for (it = plan.begin(); it != plan.end(); it++) {
      ctx.logEvent("   %s", it->ac->getName().c_str());
      if (it->params.size()) {
         Aesop::objects::const_iterator pl;
         for (pl = it->params.begin(); pl != it->params.end(); pl++) {
            ctx.logEvent(" %c", *pl);
        }
      }
      ctx.logEvent("\n");
   }
}


WarSceneAISceneImpl::WarSceneAISceneImpl(const Aesop::WorldState& initialState, const GOAPContext& context) : AISceneImpl(context),
                                                                                                              _tickCount(0),
                                                                                                              _currentEnemyTarget(nullptr),
                                                                                                              _deltaTime(0ULL),
                                                                                                              _indexInMap(-1)
{
    _planner = nullptr;
    _goalState = New Aesop::WorldState(initialState);
    _initState = New Aesop::WorldState(initialState);
    _defaultState = New Aesop::WorldState(initialState);

    MoveAction move("move", 1.0f);
    // Two parameters to this action, move-from and move-to.
    move.parameters(2);
    // Cannot move from x to x.
    move.condition(Aesop::ArgsNotEqual);        
    // Required: at() -> param 0
    move.condition(Aesop::Fact(At), 0, Aesop::Equals);
    // Required: adjacent(param 0, param 1) -> true
    move.condition(Aesop::Fact(Adjacent) % Aesop::Parameter(0) % Aesop::Parameter(1), Aesop::Equals, AI::g_predTrue);
    // Required: ignore center block
    move.condition(Aesop::Fact(At) % Aesop::Parameter(0), Aesop::NotEqual, AI::mapCenterMiddle);
    // Effect: unset at(param 0)
    move.effect(Aesop::Fact(At), Aesop::Unset);        
    // Effect: at() -> param 1
    move.effect(Aesop::Fact(At), 1, Aesop::Set);       
    registerAction(move);
}

WarSceneAISceneImpl::~WarSceneAISceneImpl()
{
    SAFE_DELETE(_goalState);
    SAFE_DELETE(_initState);
    SAFE_DELETE(_defaultState);
    SAFE_DELETE(_planner);
    _actions.clear();
}

void WarSceneAISceneImpl::registerAction(const Aesop::Action& action) {
    _actions.push_back(action);
    _actionSet.add(&_actions.back());
}

void WarSceneAISceneImpl::addEntityRef(AIEntity* entity){
    if (entity) {
        _entity = entity;
        SAFE_DELETE(_planner);
        Aesop::PVal initialLife = 100;
        Aesop::PVal friendCount = 29;
        Aesop::PVal enemyCount = 30;
        //_initState->set(Aesop::Fact(AI::Life),    initialLife);
        //_initState->set(Aesop::Fact(AI::Friends), friendCount);
        //_initState->set(Aesop::Fact(AI::Enemies), enemyCount);
        _initState->set(Aesop::Fact(AI::At), _entity->getTeam()->getTeamID() == 0 ? mapEastMiddle : mapWestMiddle);
        
        //_goalState->set(Aesop::Fact(AI::Life),    initialLife);
        //_goalState->set(Aesop::Fact(AI::Friends), friendCount);
        //_goalState->set(Aesop::Fact(AI::Enemies), 0);
        _goalState->set(Aesop::Fact(AI::At), _entity->getTeam()->getTeamID() == 0 ? mapWestMiddle : mapEastMiddle);

        _planner = New Aesop::Planner(_initState, _goalState, _defaultState, &_actionSet);
        _planner->addObject(AI::mapEastNorth);
        _planner->addObject(AI::mapEastMiddle);
        _planner->addObject(AI::mapEastSouth);
        _planner->addObject(AI::mapCenterNorth);
        _planner->addObject(AI::mapCenterMiddle);
        _planner->addObject(AI::mapCenterSouth);
        _planner->addObject(AI::mapWestNorth);
        _planner->addObject(AI::mapWestMiddle);
        _planner->addObject(AI::mapWestSouth);
    }
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
    updatePositions();

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
}

void WarSceneAISceneImpl::processData(const U64 deltaTime) {
    if (!_entity) {
        return;
    }
    if (!_planner->success()) {
        if (_planner->plan(&_context)){ 
            _context.setLogLevel(GOAPContext::LOG_LEVEL_NORMAL);
            printPlan(_context, _planner->getPlan());
            _context.setLogLevel(GOAPContext::LOG_LEVEL_NONE);
        } else {
        }
    }
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
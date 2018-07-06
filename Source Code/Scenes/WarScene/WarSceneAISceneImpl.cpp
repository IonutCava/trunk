#include "Headers/WarSceneAISceneImpl.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

static const D32 ATTACK_RADIUS = 4 * 4;

WarSceneAISceneImpl::WarSceneAISceneImpl(const GOAPContext& context) : AISceneImpl(context),
                                              _tickCount(0),
                                              _currentEnemyTarget(nullptr),
                                              _deltaTime(0ULL),
                                              _indexInMap(-1)
{
}

void WarSceneAISceneImpl::addEntityRef(AIEntity* entity){
    if (entity) {
        _entity = entity;
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
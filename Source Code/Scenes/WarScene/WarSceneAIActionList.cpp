#include "Headers/WarSceneAIActionList.h"

#include "Managers/Headers/AIManager.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

static const D32 ATTACK_RADIUS = 4 * 4;

WarSceneAIActionList::WarSceneAIActionList() : ActionList(),
                                              _tickCount(0),
                                              _navMesh(nullptr),
                                              _currentEnemyTarget(nullptr),
                                              _deltaTime(0ULL),
                                              _indexInMap(-1)
{
}

void WarSceneAIActionList::addEntityRef(AIEntity* entity){
    if(!entity) return;
    _entity = entity;
    VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    if(visualSensor){
        //_initialPosition = visualSensor->getSpatialPosition();
    }
}

void WarSceneAIActionList::processMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content){
    switch(msg){
        case CHANGE_DESTINATION_POINT: {
        }break;
    };
}

void WarSceneAIActionList::updatePositions(){
    VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
    if(visualSensor){
        _tickCount++;
        if(_tickCount == 512){
            _tickCount = 0;
        }
    }
}

void WarSceneAIActionList::processInput(const U64 deltaTime){
    updatePositions();

    if(!_navMesh) 
        return;

    _deltaTime += deltaTime;

    AICoordination* currentTeam = _entity->getTeam();
    assert(currentTeam);

    if(_entity->destinationReached()) {
        I64 currentIndex = 0;
        
        bool foundId = (_indexInMap != -1);
        if(_indexInMap == -1){
            _indexInMap = 0;
        }

        AICoordination::teamMap& team = currentTeam->getTeam();
        for_each(AICoordination::teamMap::value_type& member, team){
            if(_entity->getGUID() != member.second->getGUID()){
                _entity->sendMessage(member.second, CHANGE_DESTINATION_POINT, 0);
                if(!foundId) _indexInMap++;
            }else{
                currentIndex = member.first;
                foundId = true;
            }
        }
        
        if(currentTeam->getTeamID() == 1 && _deltaTime > getUsToSec(1.5)){ //wait 1 and a half seconds at the destination
            // team 1 moves randomly. team2 chases team 1
            _entity->updateDestination(Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(*_navMesh));
            _deltaTime = 0;
        }
    }
     
    if(!_currentEnemyTarget){
        AICoordination::teamMap& enemyTeam = _entity->getTeam()->getEnemyTeam().getTeam();
        AICoordination::teamMap::const_iterator it = enemyTeam.begin();
        I32 i = 0; 
        while(i < _indexInMap){++i; ++it;}
        _currentEnemyTarget = (*it).second;

    }

    if(currentTeam->getTeamID() == 2 && _deltaTime > getUsToMs(250)){
        AICoordination::teamMap& enemyTeam = _entity->getTeam()->getEnemyTeam().getTeam();
        AICoordination::teamMap::const_iterator it = enemyTeam.begin();

        I32 i = 0; 
        while(i < _indexInMap){++i; ++it;}

        _entity->updateDestination(_currentEnemyTarget->getPosition());

        _deltaTime = 0;
    }

    if(_currentEnemyTarget && _currentEnemyTarget->getUnitRef() && _entity->getPosition().distanceSquared(_currentEnemyTarget->getPosition()) < ATTACK_RADIUS){
        if(_currentEnemyTarget->isMoving()){
            _currentEnemyTarget->stop();
            _entity->stop();

            _currentEnemyTarget->getUnitRef()->lookAt(_entity->getPosition());
            _entity->getUnitRef()->lookAt(_currentEnemyTarget->getPosition());
         
        }else{
            const BoundingBox& bb1 = _entity->getUnitRef()->getBoundNode()->getBoundingBox();
            const BoundingBox& bb2 = _currentEnemyTarget->getUnitRef()->getBoundNode()->getBoundingBox();

            if(bb1.Collision(bb2)){
                _entity->moveBackwards();
                _currentEnemyTarget->moveBackwards();
            }
        }
    }
}

void WarSceneAIActionList::processData(const U64 deltaTime){
    if(!_navMesh)
        _navMesh = AIManager::getInstance().getNavMesh(0);
}

void WarSceneAIActionList::update(NPC* unitRef){
    if(!unitRef){
        return;
    }

    updatePositions();

    /// Updateaza informatia senzorilor
    Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);
    if(visualSensor){
        visualSensor->updatePosition(unitRef->getPosition());
    }
}
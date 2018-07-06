#include "Headers/Coordination.h"
#include "AI/Headers/AIEntity.h"

bool AICoordination::addTeamMember(AIEntity* entity) {
	_updateMutex.lock();
	if(!entity){
		_updateMutex.unlock();
		return false;
	}
	if(_team.find(entity->getGUID()) != _team.end()){
		delete _team[entity->getGUID()];
	}
	_team[entity->getGUID()] = entity;
	_updateMutex.unlock();
	return true;
}

bool AICoordination::removeTeamMember(AIEntity* entity) {
	if(!entity) return false;
	delete _team[entity->getGUID()];
	return true;
}

bool AICoordination::addEnemyTeam(teamMap& enemyTeam){
	if(!_enemyTeam.empty()){
		_enemyTeam.clear();
	}
	_enemyTeam = enemyTeam;
	return true;
}

void AICoordination::setTeamID(U32 value){
	_teamID = value;
	//for_each(teamMap::value_type member, _team){
	//	if(member.second){
	//		member.second->setTeamID(value);
	//	}
	//}
}
#include "Coordination.h"
#include "AI/AIEntity.h"

bool AICoordination::addTeamMember(AIEntity* entity) {
	boost::mutex::scoped_lock l(_updateMutex);
	if(!entity) return false;
	if(_team.find(entity->getGUID()) != _team.end()){
		delete _team[entity->getGUID()];
	}
	_team[entity->getGUID()] = entity;
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
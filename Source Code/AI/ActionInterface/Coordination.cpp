#include "Headers/Coordination.h"
#include "AI/Headers/AIEntity.h"

AICoordination::AICoordination(U32 id) : _teamID(id){
	_team.clear();
	_enemyTeam.clear();
}

bool AICoordination::addTeamMember(AIEntity* entity) {
	UpgradableReadLock ur_lock(_updateMutex);
	if(!entity){
		return false;
	}
	///If entity already belongs to this team, no need to do anything
	if(_team.find(entity->getGUID()) != _team.end()){
		return true;
	}
	UpgradeToWriteLock uw_lock(ur_lock);
	_team.insert(std::make_pair(entity->getGUID(),entity));

	return true;
}

///Removes an enitity from this list
bool AICoordination::removeTeamMember(AIEntity* entity) {
	UpgradableReadLock ur_lock(_updateMutex);
	if(!entity) return false;

	if(_team.find(entity->getGUID()) != _team.end()){
		UpgradeToWriteLock uw_lock(ur_lock);
		_team.erase(entity->getGUID());
	}
	return true;
}

bool AICoordination::addEnemyTeam(teamMap& enemyTeam){
	WriteLock w_lock(_updateMutex);
	if(!_enemyTeam.empty()){
		_enemyTeam.clear();
	}
	_enemyTeam = enemyTeam;
	return true;
}
#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Managers/Headers/AIManager.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

AITeam::AITeam(U32 id) : _teamID(id)
{
    _team.clear();
    AIManager::getInstance().registerTeam(this);
}

AITeam::~AITeam()
{
    AIManager::getInstance().unregisterTeam(this);
}

void AITeam::update(const U64 deltaTime) {
}

bool AITeam::addTeamMember(AIEntity* entity) {
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

///Removes an entity from this list
bool AITeam::removeTeamMember(AIEntity* entity) {
    UpgradableReadLock ur_lock(_updateMutex);
    if(!entity) return false;

    if(_team.find(entity->getGUID()) != _team.end()){
        UpgradeToWriteLock uw_lock(ur_lock);
        _team.erase(entity->getGUID());
    }
    return true;
}

bool AITeam::addEnemyTeam(U32 enemyTeamID) {
    if (findEnemyTeamIndex(enemyTeamID) == -1) {
        WriteLock w_lock(_updateMutex); 
        _enemyTeams.push_back(enemyTeamID);
        return true;
    }
    return false;
}

bool AITeam::removeEnemyTeam(U32 enemyTeamID) {
    I32 teamIndex = findEnemyTeamIndex(enemyTeamID);
    if (teamIndex != -1) {
        WriteLock w_lock(_updateMutex); 
        _enemyTeams.erase(_enemyTeams.begin() + teamIndex);
        return true;
    }
    return false;
}

I32 AITeam::findEnemyTeamIndex(U32 enemyTeamID) {
    ReadLock r_lock(_enemyTeamLock);

    vectorImpl<U32 >::iterator it;
    it = std::find_if(_enemyTeams.begin(), _enemyTeams.end(), [&enemyTeamID](U32 id) { 
                                                                return id == enemyTeamID; 
                                                              });

    if (it != _enemyTeams.end() ){
        return it - _enemyTeams.begin();
    }

    return -1;
}
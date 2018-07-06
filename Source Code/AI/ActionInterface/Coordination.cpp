#include "Headers/Coordination.h"
#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Managers/Headers/AIManager.h"

AICoordination::AICoordination(U32 id) : _teamID(id){
    _team.clear();
    _enemyTeam.clear();
    // attach navmeshes here
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(i);
        if(!navMesh)
            _teamCrowd[i] = NULL;
        else
            _teamCrowd[i] = New Navigation::DivideDtCrowd(navMesh);
    }
}

AICoordination::~AICoordination() {
    for(I32 i = 0; i < maxAgentRadiusCount; ++i)
        SAFE_DELETE(_teamCrowd[i]);
}

void AICoordination::resetNavMeshes() {
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(i);
        if(!_teamCrowd[i] && navMesh){
            _teamCrowd[i] = New Navigation::DivideDtCrowd(navMesh);
        }else{
            _teamCrowd[i]->setNavMesh(AIManager::getInstance().getNavMesh(i));
        }
    }
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
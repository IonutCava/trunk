#include "Headers/Coordination.h"

#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Managers/Headers/AIManager.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

AICoordination::AICoordination(U32 id) : _teamID(id){
    _team.clear();
    _enemyTeam = NULL;
    AIManager::getInstance().registerTeam(this);
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

void AICoordination::update(const U64 deltaTime){
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        if(_teamCrowd[i])
            _teamCrowd[i]->update(deltaTime);
    }
}

#pragma message("ToDo: Use proper agent radius navmeshes! -Ionut")
void AICoordination::resetNavMeshes() {
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(i);
        if(!_teamCrowd[i] && navMesh){
            _teamCrowd[i] = New Navigation::DivideDtCrowd(navMesh);
        }else if(_teamCrowd[i]){
            _teamCrowd[i]->setNavMesh(navMesh);
        }
    }
    for_each(teamMap::value_type& aiEntity, _team){
        aiEntity.second->resetCrowd(_teamCrowd[0]);
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

bool AICoordination::addEnemyTeam(AICoordination* enemyTeam){
    WriteLock w_lock(_updateMutex);

    _enemyTeam = enemyTeam;
    return true;
}
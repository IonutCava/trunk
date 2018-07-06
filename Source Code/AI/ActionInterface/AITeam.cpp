#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Managers/Headers/AIManager.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

AITeam::AITeam(U32 id) : _teamID(id){
    _team.clear();
    _enemyTeam = nullptr;
    AIManager::getInstance().registerTeam(this);
    // attach navmeshes here
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(i);
        if(!navMesh)
            _teamCrowd[i] = nullptr;
        else
            _teamCrowd[i] = New Navigation::DivideDtCrowd(navMesh);
    }
}

AITeam::~AITeam() {
    for(I32 i = 0; i < maxAgentRadiusCount; ++i)
        SAFE_DELETE(_teamCrowd[i]);
}

void AITeam::update(const U64 deltaTime){
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        if(_teamCrowd[i])
            _teamCrowd[i]->update(deltaTime);
    }
}

void AITeam::resetNavMeshes() {
    STUBBED("ToDo: Use proper agent radius navmeshes! -Ionut")
    for(I32 i = 0; i < maxAgentRadiusCount; ++i){
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(i);
        if(!_teamCrowd[i] && navMesh){
            _teamCrowd[i] = New Navigation::DivideDtCrowd(navMesh);
        }else if(_teamCrowd[i]){
            _teamCrowd[i]->setNavMesh(navMesh);
        }
    }
    FOR_EACH(teamMap::value_type& aiEntity, _team){
        aiEntity.second->resetCrowd(_teamCrowd[0]);
    }
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

///Removes an enitity from this list
bool AITeam::removeTeamMember(AIEntity* entity) {
    UpgradableReadLock ur_lock(_updateMutex);
    if(!entity) return false;

    if(_team.find(entity->getGUID()) != _team.end()){
        UpgradeToWriteLock uw_lock(ur_lock);
        _team.erase(entity->getGUID());
    }
    return true;
}

bool AITeam::addEnemyTeam(AITeam* enemyTeam){
    WriteLock w_lock(_updateMutex);

    _enemyTeam = enemyTeam;
    return true;
}
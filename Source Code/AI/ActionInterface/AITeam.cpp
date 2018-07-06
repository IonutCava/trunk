#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Managers/Headers/AIManager.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
using namespace AI;

AITeam::AITeam(U32 id) : GUIDWrapper(), 
                         _teamID(id)
{
    _team.clear();
    AIManager::getInstance().registerTeam(this);
}

AITeam::~AITeam()
{
    AIManager::getInstance().unregisterTeam(this);
    WriteLock r_lock(_crowdMutex);
    FOR_EACH(AITeamCrowd::value_type& it, _aiTeamCrowd) {
        SAFE_DELETE(it.second);
    }
    _aiTeamCrowd.clear();
}

void AITeam::addCrowd(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* navMesh) {
    DIVIDE_ASSERT(_aiTeamCrowd.find(radius) == _aiTeamCrowd.end(), "AITeam error: DtCrowd already existed for new navmesh!");
    _aiTeamCrowd.emplace(radius, New Navigation::DivideDtCrowd(navMesh));
}

void AITeam::removeCrowd(AIEntity::PresetAgentRadius radius) {
    AITeamCrowd::iterator it =  _aiTeamCrowd.find(radius);
    DIVIDE_ASSERT(it != _aiTeamCrowd.end(), "AITeam error: DtCrowd does not exist for specified navmesh!");
    SAFE_DELETE(it->second);
    _aiTeamCrowd.erase(it);
}

void AITeam::update(const U64 deltaTime) {
    // Crowds
    ReadLock r_lock(_crowdMutex);
    FOR_EACH(AITeamCrowd::value_type& it, _aiTeamCrowd) {
        it.second->update(deltaTime);
    }
    r_lock.unlock();
    WriteLock w_lock(_updateMutex);
    FOR_EACH(AITeam::TeamMap::value_type& entity, _team){
        entity.second->update(deltaTime);
    }
}

void AITeam::processInput(const U64 deltaTime) {
    WriteLock w_lock(_updateMutex);
    FOR_EACH(AITeam::TeamMap::value_type& entity, _team){
        entity.second->processInput(deltaTime);
    }
}

void AITeam::processData(const U64 deltaTime){
    WriteLock w_lock(_updateMutex);
    FOR_EACH(AITeam::TeamMap::value_type& entity, _team){
        entity.second->processData(deltaTime);
    }
}

void AITeam::init() {
    WriteLock w_lock(_updateMutex);
    FOR_EACH(AITeam::TeamMap::value_type& entity, _team){
        entity.second->init();
    }
}

void AITeam::resetCrowd() {
    WriteLock w_lock(_updateMutex); 
    FOR_EACH(AITeam::TeamMap::value_type& entity, _team){
        entity.second->resetCrowd();
    }
}

bool AITeam::addTeamMember(AIEntity* entity) {
    UpgradableReadLock ur_lock(_updateMutex);
    if (!entity){
        return false;
    }
    /// If entity already belongs to this team, no need to do anything
    if(_team.find(entity->getGUID()) != _team.end()){
        return true;
    }
    UpgradeToWriteLock uw_lock(ur_lock);
    _team.emplace(entity->getGUID(),entity);
    entity->setTeamPtr(this);
    return true;
}

///Removes an entity from this list
bool AITeam::removeTeamMember(AIEntity* entity) {
    UpgradableReadLock ur_lock(_updateMutex);
    if (!entity) {
        return false;
    }
    if(_team.find(entity->getGUID()) != _team.end()){
        UpgradeToWriteLock uw_lock(ur_lock);
        _team.erase(entity->getGUID());
    }
    return true;
}

bool AITeam::addEnemyTeam(U32 enemyTeamID) {
    if (findEnemyTeamEntry(enemyTeamID) != _enemyTeams.end()) {
        WriteLock w_lock(_updateMutex); 
        _enemyTeams.push_back(enemyTeamID);
        return true;
    }
    return false;
}

bool AITeam::removeEnemyTeam(U32 enemyTeamID) {
    vectorImpl<U32>::const_iterator it = findEnemyTeamEntry(enemyTeamID);
    if (it != _enemyTeams.end()) {
        WriteLock w_lock(_updateMutex); 
        _enemyTeams.erase(it);
        return true;
    }
    return false;
}

}; //namespace Divide
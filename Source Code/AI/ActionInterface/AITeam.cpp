#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/Headers/AIManager.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
namespace AI {

AITeam::AITeam(U32 id, AIManager& parentManager)
     : GUIDWrapper(),
       _parentManager(parentManager),
       _teamID(id)
{
    _team.clear();
    _parentManager.registerTeam(this);
}

AITeam::~AITeam()
{
    _parentManager.unregisterTeam(this);
    {
        WriteLock w1_lock(_crowdMutex);
        MemoryManager::DELETE_HASHMAP(_aiTeamCrowd);
    }
    {
        WriteLock w2_lock(_updateMutex);
        for (AITeam::TeamMap::value_type& entity : _team) {
            Attorney::AIEntityAITeam::setTeamPtr(*entity.second, nullptr);
        }
        _team.clear();
    }
}

void AITeam::addCrowd(AIEntity::PresetAgentRadius radius,
                      Navigation::NavigationMesh* navMesh) {
    DIVIDE_ASSERT(_aiTeamCrowd.find(radius) == std::end(_aiTeamCrowd),
                  "AITeam error: DtCrowd already existed for new navmesh!");
    hashAlg::emplace(_aiTeamCrowd, radius,
                     MemoryManager_NEW Navigation::DivideDtCrowd(navMesh));
}

void AITeam::removeCrowd(AIEntity::PresetAgentRadius radius) {
    AITeamCrowd::iterator it = _aiTeamCrowd.find(radius);
    DIVIDE_ASSERT(
        it != std::end(_aiTeamCrowd),
        "AITeam error: DtCrowd does not exist for specified navmesh!");
    MemoryManager::DELETE(it->second);
    _aiTeamCrowd.erase(it);
}

vectorImpl<AIEntity*> AITeam::getEntityList() const {
    vectorImpl<AIEntity*> entities;
    ReadLock r2_lock(_updateMutex);
    entities.reserve(_team.size());
    for (const AITeam::TeamMap::value_type& entity : _team) {
        entities.push_back(entity.second);
    }
    r2_lock.unlock();

    return entities;
}

bool AITeam::update(const U64 deltaTime) {
    // Crowds
    ReadLock r1_lock(_crowdMutex);
    for (AITeamCrowd::value_type& it : _aiTeamCrowd) {
        it.second->update(deltaTime);
    }
    r1_lock.unlock();

    vectorImpl<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        if (!Attorney::AIEntityAITeam::update(*entity, deltaTime)) {
            return false;
        }
    }

    return true;
}

bool AITeam::processInput(const U64 deltaTime) {
   vectorImpl<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        if (!Attorney::AIEntityAITeam::processInput(*entity, deltaTime)) {
            return false;
        }
    }

    return true;
}

bool AITeam::processData(const U64 deltaTime) {
    vectorImpl<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        if (!Attorney::AIEntityAITeam::processData(*entity, deltaTime)) {
            return false;
        }
    }

    return true;
}

void AITeam::resetCrowd() {
    vectorImpl<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        entity->resetCrowd();
    }
}

bool AITeam::addTeamMember(AIEntity* entity) {
    if (!entity) {
        return false;
    }
    /// If entity already belongs to this team, no need to do anything
    WriteLock w_lock(_updateMutex);
    if (_team.find(entity->getGUID()) != std::end(_team)) {
        return true;
    }
    hashAlg::emplace(_team, entity->getGUID(), entity);
    Attorney::AIEntityAITeam::setTeamPtr(*entity, this);

    return true;
}

/// Removes an entity from this list
bool AITeam::removeTeamMember(AIEntity* entity) {
    if (!entity) {
        return false;
    }

    WriteLock w_lock(_updateMutex);
    if (_team.find(entity->getGUID()) != std::end(_team)) {
        _team.erase(entity->getGUID());
    }
    return true;
}

bool AITeam::addEnemyTeam(U32 enemyTeamID) {
    if (findEnemyTeamEntry(enemyTeamID) == std::end(_enemyTeams)) {
        WriteLock w_lock(_updateMutex);
        _enemyTeams.push_back(enemyTeamID);
        return true;
    }
    return false;
}

bool AITeam::removeEnemyTeam(U32 enemyTeamID) {
    vectorImpl<U32>::iterator it = findEnemyTeamEntry(enemyTeamID);
    if (it != std::end(_enemyTeams)) {
        WriteLock w_lock(_updateMutex);
        _enemyTeams.erase(it);
        return true;
    }
    return false;
}

}; // namespace AI
}; // namespace Divide
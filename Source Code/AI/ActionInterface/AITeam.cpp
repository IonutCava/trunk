#include "stdafx.h"

#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/Headers/AIManager.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"

#include "Core/Headers/TaskPool.h"
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

bool AITeam::update(TaskPool& parentPool, const U64 deltaTimeUS) {
    // Crowds
    ReadLock r1_lock(_crowdMutex);
    for (AITeamCrowd::value_type& it : _aiTeamCrowd) {
        it.second->update(deltaTimeUS);
    }
    r1_lock.unlock();

    vectorImpl<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        if (!Attorney::AIEntityAITeam::update(*entity, deltaTimeUS)) {
            return false;
        }
    }

    TaskHandle updateTask = CreateTask(parentPool, DELEGATE_CBK<void, const Task&>());
    for (AIEntity* entity : entities) {
        updateTask.addChildTask(CreateTask(parentPool,
                                [entity, deltaTimeUS](const Task& parentTask)
                                {
                                    if (!Attorney::AIEntityAITeam::update(*entity, deltaTimeUS)) {
                                        //print error;
                                    }
                                }))->startTask(Task::TaskPriority::HIGH);
    }

    updateTask.startTask(Task::TaskPriority::MAX);
    updateTask.wait();

    return true;
}

bool AITeam::processInput(TaskPool& parentPool, const U64 deltaTimeUS) {
   vectorImpl<AIEntity*> entities = AITeam::getEntityList();

   TaskHandle inputTask = CreateTask(parentPool, DELEGATE_CBK<void, const Task&>());
    for (AIEntity* entity : entities) {
        inputTask.addChildTask(CreateTask(parentPool,
                               [entity, deltaTimeUS](const Task& parentTask)
                               {
                                   if (!Attorney::AIEntityAITeam::processInput(*entity, deltaTimeUS)) {
                                       //print error;
                                   }
                               }))->startTask(Task::TaskPriority::HIGH);
    }

    inputTask.startTask(Task::TaskPriority::MAX);
    inputTask.wait();

    return true;
}

bool AITeam::processData(TaskPool& parentPool, const U64 deltaTimeUS) {
    vectorImpl<AIEntity*> entities = AITeam::getEntityList();

    TaskHandle dataTask = CreateTask(parentPool, DELEGATE_CBK<void, const Task&>());
    for (AIEntity* entity : entities) {
        dataTask.addChildTask(CreateTask(parentPool,
                              [entity, deltaTimeUS](const Task& parentTask)
                              {
                                  if (!Attorney::AIEntityAITeam::processData(*entity, deltaTimeUS)) {
                                      //print error;
                                  }
                              }))->startTask(Task::TaskPriority::HIGH);
    }

    dataTask.startTask(Task::TaskPriority::MAX);
    dataTask.wait();

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
    hashAlg::insert(_team, entity->getGUID(), entity);
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
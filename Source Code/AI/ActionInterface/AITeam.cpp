#include "stdafx.h"

#include "Headers/AITeam.h"

#include "AI/Headers/AIEntity.h"
#include "AI/Headers/AIManager.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"

#include "Core/Headers/TaskPool.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {
namespace AI {

namespace {
    const U16 g_entityThreadedThreashold = 16u;
};

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
        UniqueLock<SharedMutex> w_lock(_crowdMutex);
        MemoryManager::DELETE_HASHMAP(_aiTeamCrowd);
    }
    {
        UniqueLock<SharedMutex> w_lock(_updateMutex);
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

vectorEASTL<AIEntity*> AITeam::getEntityList() const {
    //ToDo: Cache this? -Ionut
    SharedLock<SharedMutex> r2_lock(_updateMutex);

    U32 i = 0;
    vectorEASTL<AIEntity*> entities(_team.size(), nullptr);
    for (const AITeam::TeamMap::value_type& entity : _team) {
        entities[i++] = entity.second;
    }

    return entities;
}

bool AITeam::update(TaskPool& parentPool, const U64 deltaTimeUS) {
    // Crowds
    {
        SharedLock<SharedMutex> r1_lock(_crowdMutex);
        for (AITeamCrowd::value_type& it : _aiTeamCrowd) {
            it.second->update(deltaTimeUS);
        }
    }

    vectorEASTL<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        if (!Attorney::AIEntityAITeam::update(*entity, deltaTimeUS)) {
            return false;
        }
    }
    U16 entityCount = to_U16(entities.size());
    if (entityCount <= g_entityThreadedThreashold) {
        for (AIEntity* entity : entities) {
            if (!Attorney::AIEntityAITeam::update(*entity, deltaTimeUS)) {
                //print error;
            }
        }
    } else {
        auto updateIterFunction = [this, deltaTimeUS, &entities](const Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                if (!Attorney::AIEntityAITeam::update(*entities[i], deltaTimeUS)) {
                    //print error;
                }
            }
        };
        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = entityCount;
        descriptor._partitionSize = g_entityThreadedThreashold;

        parallel_for(parentPool, updateIterFunction, descriptor);
    }
    return true;
}

bool AITeam::processInput(TaskPool& parentPool, const U64 deltaTimeUS) {
    vectorEASTL<AIEntity*> entities = AITeam::getEntityList();

    U16 entityCount = to_U16(entities.size());
    if (entityCount <= g_entityThreadedThreashold) {
        for (AIEntity* entity : entities) {
            if (!Attorney::AIEntityAITeam::processInput(*entity, deltaTimeUS)) {
                //print error;
            }
        }
    } else {
        auto inputIterFunction = [this, deltaTimeUS, &entities](const Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                if (!Attorney::AIEntityAITeam::processInput(*entities[i], deltaTimeUS)) {
                    //print error;
                }
            }
        };
        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = entityCount;
        descriptor._partitionSize = g_entityThreadedThreashold;

        parallel_for(parentPool, inputIterFunction, descriptor);
    }

    return true;
}

bool AITeam::processData(TaskPool& parentPool, const U64 deltaTimeUS) {
    vectorEASTL<AIEntity*> entities = AITeam::getEntityList();

    U16 entityCount = to_U16(entities.size());
    if (entityCount <= g_entityThreadedThreashold) {
        for (AIEntity* entity : entities) {
            if (!Attorney::AIEntityAITeam::processData(*entity, deltaTimeUS)) {
                //print error;
            }
        }
    } else {
        auto dataIterFunction = [this, deltaTimeUS, &entities](const Task* parentTask, U32 start, U32 end) {
            for (U32 i = start; i < end; ++i) {
                if (!Attorney::AIEntityAITeam::processData(*entities[i], deltaTimeUS)) {
                    //print error;
                }
            }
        };

        ParallelForDescriptor descriptor = {};
        descriptor._iterCount = entityCount;
        descriptor._partitionSize = g_entityThreadedThreashold;

        parallel_for(parentPool, dataIterFunction, descriptor);
    }

    return true;
}

void AITeam::resetCrowd() {
    vectorEASTL<AIEntity*> entities = AITeam::getEntityList();
    for (AIEntity* entity : entities) {
        entity->resetCrowd();
    }
}

bool AITeam::addTeamMember(AIEntity* entity) {
    if (!entity) {
        return false;
    }
    /// If entity already belongs to this team, no need to do anything
    UniqueLock<SharedMutex> w_lock(_updateMutex);
    if (_team.find(entity->getGUID()) != std::end(_team)) {
        return true;
    }
    hashAlg::insert(_team, entity->getGUID(), entity);
    Attorney::AIEntityAITeam::setTeamPtr(*entity, this);

    return true;
}

// Removes an entity from this list
bool AITeam::removeTeamMember(AIEntity* entity) {
    if (!entity) {
        return false;
    }

    UniqueLock<SharedMutex> w_lock(_updateMutex);
    if (_team.find(entity->getGUID()) != std::end(_team)) {
        _team.erase(entity->getGUID());
    }
    return true;
}

bool AITeam::addEnemyTeam(U32 enemyTeamID) {
    if (findEnemyTeamEntry(enemyTeamID) == std::end(_enemyTeams)) {
        UniqueLock<SharedMutex> w_lock(_updateMutex);
        _enemyTeams.push_back(enemyTeamID);
        return true;
    }
    return false;
}

bool AITeam::removeEnemyTeam(U32 enemyTeamID) {
    vectorEASTL<U32>::iterator it = findEnemyTeamEntry(enemyTeamID);
    if (it != eastl::end(_enemyTeams)) {
        UniqueLock<SharedMutex> w_lock(_updateMutex);
        _enemyTeams.erase(it);
        return true;
    }
    return false;
}

}; // namespace AI
}; // namespace Divide
#include "stdafx.h"

#include "config.h"

#include "Headers/AIManager.h"

#include "AI/ActionInterface/Headers/AITeam.h"
#include "AI/PathFinding/Headers/DivideRecast.h"
#include "AI/PathFinding/NavMeshes/Headers/NavMesh.h"

#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

using namespace AI;

AIManager::AIManager(Scene& parentScene, TaskPool& pool)
    : SceneComponent(parentScene),
      _parentPool(pool),
      _activeTask(nullptr),
      _navMeshDebugDraw(false),
      _pauseUpdate(true),
      _updating(false),
      _deltaTimeUS(0ULL),
      _currentTimeUS(0ULL),
      _previousTimeUS(0ULL)
{
    _shouldStop = false;
    _running = false;
}

AIManager::~AIManager()
{
    destroy();
}

/// Clear up any remaining AIEntities
void AIManager::destroy() {
    {
        UniqueLock w_lock(_updateMutex);
        for (AITeamMap::value_type& entity : _aiTeams) {
            MemoryManager::DELETE(entity.second);
        }
        _aiTeams.clear();
    }
    {
        UniqueLock w_lock(_navMeshMutex);
        for (NavMeshMap::value_type& it : _navMeshes) {
            MemoryManager::DELETE(it.second);
        }
        _navMeshes.clear();
    }
}

void AIManager::update(const U64 deltaTimeUS) {
     constexpr U64 updateFreqUS = Time::SecondsToMicroseconds(1) / (Config::TARGET_FRAME_RATE / Config::TICK_DIVISOR);

    _currentTimeUS += deltaTimeUS;
    if (_currentTimeUS >= _previousTimeUS + updateFreqUS && !shouldStop()) {
        _running = true;
        /// use internal delta time calculations
        _deltaTimeUS = _currentTimeUS - _previousTimeUS;
        {
            /// Lock the entities during update() adding or deleting entities is
            /// suspended until this returns
            UniqueLock r_lock(_updateMutex);
            if (!_aiTeams.empty() && !_pauseUpdate) {
                _updating = true;
                if (_sceneCallback) {
                    _sceneCallback();
                }

                if (processInput(deltaTimeUS)) {  // sensors
                    if (processData(deltaTimeUS)) {  // think
                        updateEntities(deltaTimeUS);  // react
                    }
                }

                _updating = false;
            }
        }
        _previousTimeUS = _currentTimeUS;
        _running = false;
    }
}

bool AIManager::processInput(const U64 deltaTimeUS) {  // sensors
    for (AITeamMap::value_type& team : _aiTeams) {
        if (!team.second->processInput(_parentPool, deltaTimeUS)) {
            return false;
        }
    }
    return true;
}

bool AIManager::processData(const U64 deltaTimeUS) {  // think
    for (AITeamMap::value_type& team : _aiTeams) {
        if (!team.second->processData(_parentPool, deltaTimeUS)) {
            return false;
        }
    }
    return true;
}

bool AIManager::updateEntities(const U64 deltaTimeUS) {  // react
    for (AITeamMap::value_type& team : _aiTeams) {
        if (!team.second->update(_parentPool, deltaTimeUS)) {
            return false;
        }
    }
    return true;
}

bool AIManager::registerEntity(U32 teamID, AIEntity* entity) {
    UniqueLock w_lock(_updateMutex);
    AITeamMap::const_iterator it = _aiTeams.find(teamID);
    DIVIDE_ASSERT(it != std::end(_aiTeams),
                  "AIManager error: attempt to register an AI Entity to a "
                  "non-existent team!");
    it->second->addTeamMember(entity);
    return true;
}

void AIManager::unregisterEntity(AIEntity* entity) {
    for (AITeamMap::value_type& team : _aiTeams) {
        unregisterEntity(team.second->getTeamID(), entity);
    }
}

void AIManager::unregisterEntity(U32 teamID, AIEntity* entity) {
    UniqueLock w_lock(_updateMutex);
    AITeamMap::const_iterator it = _aiTeams.find(teamID);
    DIVIDE_ASSERT(it != std::end(_aiTeams),
                  "AIManager error: attempt to remove an AI Entity from a "
                  "non-existent team!");
    it->second->removeTeamMember(entity);
}

bool AIManager::addNavMesh(AIEntity::PresetAgentRadius radius,
                           Navigation::NavigationMesh* const navMesh) {
    {
        UniqueLock w_lock(_navMeshMutex);
        NavMeshMap::iterator it = _navMeshes.find(radius);
        DIVIDE_ASSERT(it == std::end(_navMeshes),
                      "AIManager error: NavMesh for specified dimensions already "
                      "exists. Remove it first!");
        DIVIDE_ASSERT(navMesh != nullptr,
                      "AIManager error: Invalid navmesh specified!");

        navMesh->debugDraw(_navMeshDebugDraw);
        hashAlg::insert(_navMeshes, radius, navMesh);
    }
    {
        UniqueLock w_lock(_updateMutex);
        for (AITeamMap::value_type& team : _aiTeams) {
            team.second->addCrowd(radius, navMesh);
        }
    }
    return true;
}

void AIManager::destroyNavMesh(AIEntity::PresetAgentRadius radius) {
    {
        UniqueLock w_lock(_navMeshMutex);
        NavMeshMap::iterator it = _navMeshes.find(radius);
        DIVIDE_ASSERT(it != std::end(_navMeshes),
                      "AIManager error: Can't destroy NavMesh for specified radius "
                      "(NavMesh not found)!");
        MemoryManager::DELETE(it->second);
        _navMeshes.erase(it);
    }

    {
        UniqueLock w_lock(_updateMutex);
        for (AITeamMap::value_type& team : _aiTeams) {
            team.second->removeCrowd(radius);
        }
    }
}

void AIManager::registerTeam(AITeam* const team) {
    UniqueLock w_lock(_updateMutex);
    U32 teamID = team->getTeamID();
    DIVIDE_ASSERT(_aiTeams.find(teamID) == std::end(_aiTeams),
                  "AIManager error: attempt to double register an AI team!");

    hashAlg::insert(_aiTeams, teamID, team);
}

void AIManager::unregisterTeam(AITeam* const team) {
    UniqueLock w_lock(_updateMutex);
    U32 teamID = team->getTeamID();
    AITeamMap::iterator it = _aiTeams.find(teamID);
    DIVIDE_ASSERT(it != std::end(_aiTeams),
                  "AIManager error: attempt to unregister an invalid AI team!");
    _aiTeams.erase(it);
}

void AIManager::toggleNavMeshDebugDraw(bool state) {
    UniqueLock w_lock(_navMeshMutex);
    for (NavMeshMap::value_type& it : _navMeshes) {
        it.second->debugDraw(state);
    }

    _navMeshDebugDraw = state;
}

void AIManager::debugDraw(GFX::CommandBuffer& bufferInOut, bool forceAll) {
    UniqueLock w_lock(_navMeshMutex);
    for (NavMeshMap::value_type& it : _navMeshes) {
        it.second->update(_deltaTimeUS);
        if (forceAll || it.second->debugDraw()) {
            bufferInOut.add(it.second->draw());
        }
    }
}

bool AIManager::shouldStop() const {
    if (_activeTask != nullptr) {
        Stop(*_activeTask);
        Wait(*_activeTask);
    }
    return _shouldStop;
}
};  // namespace Divide
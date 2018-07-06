#include "Headers/AIManager.h"

#include "AI/ActionInterface/Headers/AITeam.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

#include "Core/Headers/ApplicationTimer.h"

namespace Divide {

using namespace AI;

AIManager::AIManager() : _navMeshDebugDraw(false),
                         _pauseUpdate(true),
                         _updating(false),
                         _deltaTime(0ULL),
                         _currentTime(0ULL),
                         _previousTime(0ULL)
{
    Navigation::DivideRecast::createInstance();
}

AIManager::~AIManager() 
{
    Destroy();
}

///Clear up any remaining AIEntities
void AIManager::Destroy() {
    {
        WriteLock w_lock(_updateMutex);
        for (AITeamMap::value_type& entity : _aiTeams) {
            MemoryManager::DELETE( entity.second );
        }
        _aiTeams.clear();
    }
    {
        WriteLock w_lock(_navMeshMutex);
        for (NavMeshMap::value_type& it : _navMeshes) {
            MemoryManager::DELETE( it.second );
        }
        _navMeshes.clear();

        Navigation::DivideRecast::destroyInstance();
    }
}

void AIManager::update() {
    ///Lock the entities during update() adding or deleting entities is suspended until this returns
    ReadLock r_lock(_updateMutex);
    /// use internal delta time calculations
    _previousTime = _currentTime;
    _currentTime  = Time::ElapsedMicroseconds();
    _deltaTime = _currentTime - _previousTime;
    if (_aiTeams.empty() || _pauseUpdate) {
        return; //nothing to do
    }
    _updating = true;
    if (_sceneCallback) {
        _sceneCallback();
    }
    r_lock.unlock();

    processInput(_deltaTime);  //sensors
    processData(_deltaTime);   //think
    updateEntities(_deltaTime);//react
    _updating = false;
    return;
}

void AIManager::signalInit() {
    ReadLock r_lock(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->init();
    }
}

void AIManager::processInput(const U64 deltaTime) {  //sensors
    ReadLock r_lock(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->processInput( deltaTime );
    }
}

void AIManager::processData(const U64 deltaTime) {   //think
    ReadLock r_lock(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->processData( deltaTime );
    }
}

void AIManager::updateEntities(const U64 deltaTime){//react
    ReadLock r_lock(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->update( deltaTime );
    }
}

bool AIManager::registerEntity(U32 teamId, AIEntity* entity) {
    WriteLock w_lock(_updateMutex);
    AITeamMap::const_iterator it = _aiTeams.find(teamId);
    DIVIDE_ASSERT(it != std::end(_aiTeams), "AIManager error: attempt to register an AI Entity to a non-existent team!");
    it->second->addTeamMember(entity);
    return true;
}

void AIManager::unregisterEntity(AIEntity* entity) { 
    for (AITeamMap::value_type& team : _aiTeams) {
        unregisterEntity( team.second->getTeamID(), entity );
    }
}

void AIManager::unregisterEntity(U32 teamId, AIEntity* entity) {
    WriteLock w_lock(_updateMutex);
    AITeamMap::const_iterator it = _aiTeams.find(teamId);
    DIVIDE_ASSERT(it != std::end(_aiTeams), "AIManager error: attempt to remove an AI Entity from a non-existent team!");
    it->second->removeTeamMember(entity);
}

bool AIManager::addNavMesh(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* const navMesh) {
    WriteLock w_lock(_navMeshMutex);
    NavMeshMap::iterator it = _navMeshes.find(radius);
    DIVIDE_ASSERT(it == std::end(_navMeshes), "AIManager error: NavMesh for specified dimensions already exists. Remove it first!");
    DIVIDE_ASSERT(navMesh != nullptr, "AIManager error: Invalid navmesh specified!");

    navMesh->debugDraw(_navMeshDebugDraw);
    hashAlg::emplace(_navMeshes, radius, navMesh);
    w_lock.unlock();

    WriteLock w_lock2(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->addCrowd( radius, navMesh );
    }

    return true;
}

void AIManager::destroyNavMesh(AIEntity::PresetAgentRadius radius) {
    WriteLock w_lock(_navMeshMutex);
    NavMeshMap::iterator it = _navMeshes.find(radius);
    DIVIDE_ASSERT(it != std::end(_navMeshes), "AIManager error: Can't destroy NavMesh for specified radius (NavMesh not found)!");
    MemoryManager::DELETE( it->second );
    _navMeshes.erase(it);
    w_lock.unlock();
    
    WriteLock w_lock2(_updateMutex);
    for (AITeamMap::value_type& team : _aiTeams) {
        team.second->removeCrowd( radius );
    }
}

void AIManager::registerTeam(AITeam* const team) {
    WriteLock w_lock(_updateMutex);
    U32 teamID = team->getTeamID();
    DIVIDE_ASSERT(_aiTeams.find(teamID) == std::end(_aiTeams), "AIManager error: attempt to double register an AI team!");

    hashAlg::insert(_aiTeams, hashAlg::makePair(teamID, team));
}

void AIManager::unregisterTeam(AITeam* const team) {
    WriteLock w_lock(_updateMutex);
    U32 teamID = team->getTeamID();
    AITeamMap::iterator it = _aiTeams.find(teamID);
    DIVIDE_ASSERT(it != std::end(_aiTeams), "AIManager error: attempt to unregister an invalid AI team!");
    _aiTeams.erase(it);
}

void AIManager::toggleNavMeshDebugDraw(bool state) {
    WriteLock w_lock(_navMeshMutex);
    for (NavMeshMap::value_type& it : _navMeshes) {
        it.second->debugDraw( state );
    }

    _navMeshDebugDraw = state;
}

void AIManager::debugDraw(bool forceAll) {
    WriteLock w_lock(_navMeshMutex);
    for (NavMeshMap::value_type& it : _navMeshes) {
        it.second->update( _deltaTime );
        if ( forceAll || it.second->debugDraw() ) {
            it.second->render();
        }
    }
}

}; //namespace Divide
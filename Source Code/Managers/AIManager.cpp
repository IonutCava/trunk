#include "Headers/AIManager.h"

#include "AI/PathFinding/Headers/DivideRecast.h"
#include "AI/PathFinding/Headers/DivideCrowd.h"

using namespace AI;

AIManager::AIManager() : _navMeshDebugDraw(false), _pauseUpdate(true), _deltaTime(0ULL), _currentTime(0ULL), _previousTime(0ULL)
{
    _updating = false;
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
        FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
            SAFE_DELETE(entity.second);
        }
        _aiEntities.clear();
    }
    {
        WriteLock w_lock(_navMeshMutex);
        FOR_EACH(AITeamCrowdList::value_type& it, _aiTeamCrowds) {
            FOR_EACH(AITeamCrowd::value_type& it2, it.second) {
                SAFE_DELETE(it2.second);
            }
            it.second.clear();
        }
        _aiTeamCrowds.clear();

        FOR_EACH(NavMeshMap::value_type& it, _navMeshes){
             SAFE_DELETE(it.second);
        }
        _navMeshes.clear();

        Navigation::DivideRecast::destroyInstance();
    }
}

U8 AIManager::update(){
    ///Lock the entities during update() adding or deleting entities is suspended until this returns
    ReadLock r_lock(_updateMutex);
    /// use internal delta time calculations
    _previousTime = _currentTime;
    _currentTime  = GETUSTIME();
    _deltaTime = _currentTime - _previousTime;
    if(_aiEntities.empty() || _pauseUpdate){
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        return 1; //nothing to do
    }
    _updating = true;
    if (!_sceneCallback.empty()) {
        _sceneCallback();
    }
    processInput(_deltaTime);  //sensors
    processData(_deltaTime);   //think
    updateEntities(_deltaTime);//react
    _updating = false;
    return 0;
}

void AIManager::signalInit() {
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->init();
    }
}

void AIManager::processInput(const U64 deltaTime){  //sensors
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->processInput(deltaTime);
    }
}

void AIManager::processData(const U64 deltaTime){   //think
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->processData(deltaTime);
    }
}

void AIManager::updateEntities(const U64 deltaTime){//react
    // Crowds
    FOR_EACH(AITeamCrowdList::value_type& it, _aiTeamCrowds) {
        FOR_EACH(AITeamCrowd::value_type& it2, it.second) {
            it2.second->update(deltaTime);
        }
    }
    // Teams
    FOR_EACH(AITeamMap::value_type& team, _aiTeams){
        team.second->update(deltaTime);
    }
    // Entities
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->update(deltaTime);
    }
}

bool AIManager::addEntity(AIEntity* entity){
    WriteLock w_lock(_updateMutex);
    if(_aiEntities.find(entity->getGUID()) != _aiEntities.end()){
        SAFE_UPDATE(_aiEntities[entity->getGUID()], entity);
    }else{
        _aiEntities.insert(std::make_pair(entity->getGUID(),entity));
    }

    return true;
}

void AIManager::destroyEntity(U32 guid){
    WriteLock w_lock(_updateMutex);
    SAFE_DELETE(_aiEntities[guid]);
    _aiEntities.erase(guid);
}

bool AIManager::addNavMesh(AIEntity::PresetAgentRadius radius, Navigation::NavigationMesh* const navMesh) {
    WriteLock w_lock(_navMeshMutex);
    NavMeshMap::iterator it = _navMeshes.find(radius);
    DIVIDE_ASSERT(it == _navMeshes.end(), "AIManager error: Nnv mesh for specified dimensions already exists. Remove it first!");
    DIVIDE_ASSERT(navMesh != nullptr, "AIManager error: Invalid navmesh specified!");
    navMesh->debugDraw(_navMeshDebugDraw);
    _navMeshes.insert(std::make_pair(radius, navMesh));
    FOR_EACH(AITeamCrowdList::value_type& it, _aiTeamCrowds) {
        AITeamCrowd::iterator it2 = it.second.find(radius);
        DIVIDE_ASSERT(it2 == it.second.end(), "AIManager error: DTCrowd already existed for new navmesh!");
        it.second.insert(std::make_pair(radius, New Navigation::DivideDtCrowd(navMesh)));
    }
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->resetCrowd();
    }

    return true;
}

void AIManager::destroyNavMesh(AIEntity::PresetAgentRadius radius) {
    WriteLock w_lock(_navMeshMutex);
    NavMeshMap::iterator it = _navMeshes.find(radius);
    if (it != _navMeshes.end()) {
        FOR_EACH(AITeamCrowdList::value_type& it, _aiTeamCrowds) {
            AITeamCrowd::iterator it2 = it.second.find(radius);
            if (it2 != it.second.end()) {
                SAFE_DELETE(it2->second);
                it.second.erase(it2);
            }
        }
        FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
            entity.second->resetCrowd();
        }
    }
}

void AIManager::registerTeam(AITeam* const team) {
    U32 teamID = team->getTeamID();
    DIVIDE_ASSERT(_aiTeams.find(teamID) == _aiTeams.end(), "AIManager error: attempt to double register an AI team!");
    _aiTeams.insert(std::make_pair(teamID, team));
    _aiTeamCrowds.insert(std::make_pair(teamID, AITeamCrowd()));
}

void AIManager::unregisterTeam(AITeam* const team) {
    U32 teamID = team->getTeamID();
    AITeamCrowdList::iterator it = _aiTeamCrowds.find(teamID);
    DIVIDE_ASSERT(it != _aiTeamCrowds.end(), "AIManager error: Unregister team failed. Team not found!");
    FOR_EACH(AITeamCrowd::value_type& it2, it->second) {
        SAFE_DELETE(it2.second);
    }
    it->second.clear();
    _aiTeamCrowds.erase(it);
}

void AIManager::toggleNavMeshDebugDraw(Navigation::NavigationMesh* navMesh, bool state) {
    navMesh->debugDraw(state);
    toggleNavMeshDebugDraw(state);
}

void AIManager::toggleNavMeshDebugDraw(bool state) {
    WriteLock w_lock(_navMeshMutex);
    FOR_EACH(NavMeshMap::value_type& it, _navMeshes){
        it.second->debugDraw(state);
    }

    _navMeshDebugDraw = state;
}

void AIManager::debugDraw(bool forceAll) {
    WriteLock w_lock(_navMeshMutex);
    FOR_EACH(NavMeshMap::value_type& it, _navMeshes){
        it.second->update(_deltaTime);
        if(forceAll || it.second->debugDraw()){
            it.second->render();
        }
    }
}
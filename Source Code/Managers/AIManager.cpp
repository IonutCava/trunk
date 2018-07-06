#include "Headers/AIManager.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

AIManager::AIManager() : _navMeshDebugDraw(false), _pauseUpdate(true), _updateNavMeshes(false), _deltaTime(0ULL), _currentTime(0ULL), _previousTime(0ULL)
{
    Navigation::DivideRecast::createInstance();
}

AIManager::~AIManager() 
{
    Destroy();
}

///Clear up any remaining AIEntities
void AIManager::Destroy(){
    WriteLock w_lock(_updateMutex);
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        SAFE_DELETE(entity.second);
    }
    _aiEntities.clear();
    for(Navigation::NavigationMesh*& navMesh : _navMeshes){
         SAFE_DELETE(navMesh);
    }
    _navMeshes.clear();

    Navigation::DivideRecast::destroyInstance();
}

U8 AIManager::update(){
    ///Lock the entities during update() adding or deleting entities is suspended until this returns
    ReadLock r_lock(_updateMutex);
    /// use internal delta time calculations
    _previousTime = _currentTime;
    _currentTime  = GETUSTIME();
    _deltaTime = _currentTime - _previousTime;

    if(_aiEntities.empty() || _pauseUpdate){
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        return 1; //nothing to do
    }
    if(!_sceneCallback.empty())
        _sceneCallback();

    processInput(_deltaTime);  //sensors
    processData(_deltaTime);   //think
    updateEntities(_deltaTime);//react
    return 0;
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
    FOR_EACH(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->update(deltaTime);
    }
    
    FOR_EACH(AITeamMap::value_type& team, _aiTeams){
        team.second->update(deltaTime);
    }

    if(_updateNavMeshes){
        ReadLock w_lock(_navMeshMutex);
        FOR_EACH(AITeamMap::value_type& team, _aiTeams){
            team.second->resetNavMeshes();
        }
        _updateNavMeshes = false;
    }
}

bool AIManager::addEntity(AIEntity* entity){
    WriteLock w_lock(_updateMutex);
    if(_aiEntities.find(entity->_GUID) != _aiEntities.end()){
        SAFE_UPDATE(_aiEntities[entity->_GUID], entity);
    }else{
        _aiEntities.insert(std::make_pair(entity->_GUID,entity));
    }

    return true;
}

void AIManager::destroyEntity(U32 guid){
    WriteLock w_lock(_updateMutex);
    SAFE_DELETE(_aiEntities[guid]);
    _aiEntities.erase(guid);
}

/// Example nav mesh code:
/*
    PRINT_FN("Selecting 2 random points on the current navmesh and finding a path between them: ");
    CONSOLE_TIMESTAMP_OFF();
    vec3<F32> temp1 = Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(*navMesh);
    vec3<F32> temp2 = Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(*navMesh);
    PRINT_FN("Random point A [X: %5.2f | Y: %5.2f | Z:%5.2f]", temp1.x, temp1.y, temp1.z);
    PRINT_FN("Random point B [X: %5.2f | Y: %5.2f | Z:%5.2f]", temp2.x, temp2.y, temp2.z);
    Navigation::PathErrorCode err = Navigation::DivideRecast::getInstance().FindPath(*navMesh, temp1,temp2,0,1);
    if(err == Navigation::PATH_ERROR_NONE){
        U8 spaceCtr = 0;
        vectorImpl<vec3<F32> > path = Navigation::DivideRecast::getInstance().getPath(0);
        for(vectorImpl<vec3<F32> >::const_iterator it = path.begin(); it != path.end(); ++it, ++spaceCtr){
            const vec3<F32>& currentNode = *it;
            for(U8 j = 0; j < spaceCtr; ++j)
                PRINT_F(" ");
            PRINT_FN("Node %d : [X: %5.2f | Y: %5.2f | Z:%5.2f]",spaceCtr, currentNode.x, currentNode.y, currentNode.z);
        }
    }
    CONSOLE_TIMESTAMP_ON();
*/
bool AIManager::addNavMesh(Navigation::NavigationMesh* const navMesh){
    navMesh->debugDraw(_navMeshDebugDraw);
    WriteLock w_lock(_navMeshMutex);
    _navMeshes.push_back(navMesh);
    _updateNavMeshes = true;
    return true;
}

void AIManager::destroyNavMesh(Navigation::NavigationMesh* const navMesh){
    WriteLock w_lock(_navMeshMutex);
    for(vectorImpl<Navigation::NavigationMesh* >::iterator it =  _navMeshes.begin();
                                                           it != _navMeshes.end();
                                                         ++it){
        if((*it)->getGUID() == navMesh->getGUID()){
            SAFE_DELETE((*it));
            _navMeshes.erase(it);
            return;
        }
    }
    _updateNavMeshes = true;
}

void AIManager::toggleNavMeshDebugDraw(Navigation::NavigationMesh* navMesh, bool state) {
    navMesh->debugDraw(state);
    toggleNavMeshDebugDraw(state);
}

void AIManager::toggleNavMeshDebugDraw(bool state) {
    WriteLock w_lock(_navMeshMutex);
    for(Navigation::NavigationMesh* navMesh : _navMeshes){
        navMesh->debugDraw(state);
    }
    _navMeshDebugDraw = state;
}

void AIManager::debugDraw(bool forceAll){
    ReadLock r_lock(_navMeshMutex);
    for(Navigation::NavigationMesh* navMesh : _navMeshes){
        navMesh->update(_deltaTime);
        if(forceAll || navMesh->debugDraw()){
            navMesh->render();
        }
    }
}
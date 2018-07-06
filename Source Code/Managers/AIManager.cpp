#include "Headers/AIManager.h"
#include "AI/PathFinding/Headers/NavigationPath.h"

AIManager::AIManager() : _navMeshDebugDraw(false), _pauseUpdate(true), _deltaTimeMS(0)
{
    Navigation::DivideRecast::createInstance();
}

///Clear up any remaining AIEntities
void AIManager::Destroy(){
    WriteLock w_lock(_updateMutex);
    for_each(AIEntityMap::value_type& entity, _aiEntities){
        SAFE_DELETE(entity.second);
    }
    _aiEntities.clear();
    for_each(Navigation::NavigationMesh* navMesh, _navMeshes){
         SAFE_DELETE(navMesh);
    }
    _navMeshes.clear();

    Navigation::DivideRecast::destroyInstance();
}

U8 AIManager::tick(){
    ///Lock the entities during tick() adding or deleting entities is suspended until this returns
    ReadLock r_lock(_updateMutex);
    _deltaTimeMS = GETMSTIME() - _deltaTimeMS;
    if(_aiEntities.empty() || _pauseUpdate){
        boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        return 1; //nothing to do
    }
    if(!_sceneCallback.empty())
        _sceneCallback();

    processInput();  //sensors
    processData();   //think
    updateEntities();//react
    return 0;
}

void AIManager::processInput(){  //sensors
    for_each(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->processInput();
    }
}

void AIManager::processData(){   //think
    for_each(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->processData();
    }
}

void AIManager::updateEntities(){//react
    for_each(AIEntityMap::value_type& entity, _aiEntities){
        entity.second->update();
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

bool AIManager::addNavMesh(Navigation::NavigationMesh* const navMesh){
    navMesh->debugDraw(_navMeshDebugDraw);
    WriteLock w_lock(_navMeshMutex);
    _navMeshes.push_back(navMesh);
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
}

void AIManager::toggleNavMeshDebugDraw(bool state) {
    WriteLock w_lock(_navMeshMutex);
    for_each(Navigation::NavigationMesh* navMesh, _navMeshes){
        navMesh->debugDraw(state);
    }
    _navMeshDebugDraw = state;
}

void AIManager::debugDraw(bool forceAll){
    ReadLock r_lock(_navMeshMutex);
    for_each(Navigation::NavigationMesh* navMesh, _navMeshes){
        navMesh->tick(_deltaTimeMS);
        if(forceAll || navMesh->debugDraw()){
            navMesh->render();
        }
    }
}
#include "Headers/AIManager.h"

U8 AIManager::tick(){
	processInput();  //sensors
	processData();   //think
	updateEntities();//react
	return 0;
}

bool AIManager::addEntity(AIEntity* entity){
	WriteLock w_lock(_updateMutex);
	if(_aiEntities.find(entity->_GUID) != _aiEntities.end()){
		delete _aiEntities[entity->_GUID];
		_aiEntities[entity->_GUID] = entity;
	}else{
		_aiEntities.insert(std::make_pair(entity->_GUID,entity));
	}

	return true;
}

void AIManager::processInput(){  //sensors
	ReadLock r_lock(_updateMutex);
	for_each(AIEntityMap::value_type& entity, _aiEntities){
		entity.second->processInput();
	}
}

void AIManager::processData(){   //think
	ReadLock r_lock(_updateMutex);
	for_each(AIEntityMap::value_type& entity, _aiEntities){
		entity.second->processData();
	}
}

void AIManager::updateEntities(){//react
	ReadLock r_lock(_updateMutex);
	for_each(AIEntityMap::value_type& entity, _aiEntities){
		entity.second->update();
	}
}

void AIManager::destroyEntity(U32 guid){
	WriteLock w_lock(_updateMutex);
	SAFE_DELETE(_aiEntities[guid]);
	_aiEntities.erase(guid);
}

///Clear up any remaining AIEntities
void AIManager::Destroy(){
	WriteLock w_lock(_updateMutex);
	for_each(AIEntityMap::value_type& entity, _aiEntities){
		SAFE_DELETE(entity.second);
	}
	_aiEntities.clear();
}
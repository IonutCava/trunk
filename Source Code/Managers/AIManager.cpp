#include "Headers/AIManager.h"
#pragma message("ToDo: Maybe create the \"Unit\" class and agregate it with AIEntity? -Ionut")
U8 AIManager::tick(){
	///Lock the entities during tick() adding or deleting entities is suspended until this returns
	ReadLock r_lock(_updateMutex);
	if(_aiEntities.empty()){
		return 1; //nothing to do
	}
	if(!_sceneCallback.empty()) _sceneCallback();
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

///Clear up any remaining AIEntities
void AIManager::Destroy(){
	WriteLock w_lock(_updateMutex);
	for_each(AIEntityMap::value_type& entity, _aiEntities){
		SAFE_DELETE(entity.second);
	}
	_aiEntities.clear();
}
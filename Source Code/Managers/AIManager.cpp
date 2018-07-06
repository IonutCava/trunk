#include "AIManager.h"

U8 AIManager::tick(){
	processInput();  //sensors
	processData();   //think
	updateEntities();//react
	return 0;
}

bool AIManager::addEntity(AIEntity* entity){
	_updateMutex.lock();
	if(_aiEntities.find(entity->_GUID) != _aiEntities.end()){
		delete _aiEntities[entity->_GUID];

	}
	_aiEntities[entity->_GUID] = entity;
	_updateMutex.unlock();
	return true;
}

void AIManager::processInput(){  //sensors
	for_each(AIEntityMap::value_type entity, _aiEntities){
		entity.second->processInput();
	}
}

void AIManager::processData(){   //think
	for_each(AIEntityMap::value_type entity, _aiEntities){
		entity.second->processData();
	}
}

void AIManager::updateEntities(){//react
	for_each(AIEntityMap::value_type entity, _aiEntities){
		entity.second->update();
	}
}

void AIManager::Destroy(){
	_updateMutex.lock();
	for_each(AIEntityMap::value_type entity, _aiEntities){
		delete entity.second;
	}
	_aiEntities.clear();
	_updateMutex.unlock();
}
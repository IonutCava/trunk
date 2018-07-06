#include "AIManager.h"

U8 AIManager::tick(){
	processInput();  //sensors
	processData();   //think
	updateEntities();//react
	return 0;
}

void AIManager::processInput(){  //sensors
	foreach(AIEntityMap::value_type entity, aiEntities){
		entity.second->processInput();
	}
}

void AIManager::processData(){   //think
	foreach(AIEntityMap::value_type entity, aiEntities){
		entity.second->processData();
	}
}

void AIManager::updateEntities(){//react
	foreach(AIEntityMap::value_type entity, aiEntities){
		entity.second->update();
	}
}
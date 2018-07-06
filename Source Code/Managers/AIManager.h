#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "resource.h"
#include "AI/AIEntity.h"

DEFINE_SINGLETON(AIManager)
	typedef unordered_map<U32, AIEntity*> AIEntityMap;

public:
	U8 tick();
	bool addEntity(AIEntity* entity);
	void Destroy();

private:
	void processInput();  //sensors
	void processData();   //think
	void updateEntities();//react

private:
	//ToDo: Maybe create the "Unit" class and agregate it with AIEntity? -Ionut
	AIEntityMap _aiEntities;
	boost::mutex _updateMutex;

END_SINGLETON

#endif
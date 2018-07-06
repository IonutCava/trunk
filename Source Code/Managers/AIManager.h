#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "resource.h"
#include "AI/AIEntity.h"

DEFINE_SINGLETON(AIManager)
	typedef unordered_map<U32, AIEntity*> AIEntityMap;

public:
	U8 tick();
	
private:
	void processInput();  //sensors
	void processData();   //think
	void updateEntities();//react
	//ToDo: Maybe create the "Unit" class and agregate it with AIEntity? -Ionut
	AIEntityMap aiEntities;
END_SINGLETON
#endif
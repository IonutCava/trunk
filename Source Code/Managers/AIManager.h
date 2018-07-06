#ifndef _AI_MANAGER_H_
#define _AI_MANAGER_G_

#include "resource.h"
#include "AI/AIEntity.h"

DEFINE_SINGLETON(AIManager)


public:
	void processInput();  //sensors
	void processData();   //think
	void updateEntities();//react

private:
	//ToDo: Maybe create the "Unit" class and agregate it with AIEntity? -Ionut
	unordered_map<U32, AIEntity> aiEntities;
END_SINGLETON
#endif
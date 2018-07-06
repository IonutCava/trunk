#ifndef _AI_TENIS_SCENE_AI_ACTION_LIST_H_
#define _AI_TENIS_SCENE_AI_ACTION_LIST_H_
#include "AI/ActionInterface/ActionList.h"

enum AI_MSG{
	REQUEST_DISTANCE_TO_TARGET = 0,
	RECEIVE_DISTANCE_TO_TARGET = 1,
	LOVESTE_MINGEA = 2,
	NU_LOVI_MINGEA = 3
};

class AITenisSceneAIActionList : public ActionList{
	typedef unordered_map<AIEntity*, F32 > membruDistantaMap;
public:
	AITenisSceneAIActionList(SceneGraphNode* target);
	void processData();
	void processInput();
	void update(SceneGraphNode* node);
	void addEntityRef(AIEntity* entity);
	void processMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);

private:
	void updatePositions();

private:
	SceneGraphNode* _node;
	SceneGraphNode* _target;
	vec3 _pozitieMinge, _pozitieMingeAnterioara, _pozitieEntitate, _pozitieInitiala;
	membruDistantaMap _membruDistanta;
	bool _atacaMingea, _mingeSpreEchipa2,_stopJoc;
	U16 _tickCount;
};

#endif
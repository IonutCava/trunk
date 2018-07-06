#ifndef _AI_ACTION_LIST_H_
#define _AI_ACTION_LIST_H_
#include "AI/AIEntity.h"

enum AI_MSG;

class SceneGraphNode;
class ActionList{
public:
	ActionList() {};
	virtual void processData() = 0;
	virtual void processInput() = 0;
	virtual void update(SceneGraphNode* node) = 0;
	virtual void addEntityRef(AIEntity* entity) = 0;
	virtual void processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content) = 0;
protected:
	AIEntity*  _entity;

};

#endif
#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "resource.h"
#include "Sensors/VisualSensor.h"
#include "Sensors/CommunicationSensor.h"
#include "ActionInterface/Coordination.h"

class ActionList;
class SceneGraphNode;

class AIEntity{
	friend class AIManager;
public:
	AIEntity(const std::string& name);
	
	void processInput();
	void processData();
	void update();

	bool attachNode(SceneGraphNode* const node) {_node = node; return true;}
	bool addSensor(SENSOR_TYPE type, Sensor* sensor);
	bool addFriend(AIEntity* entity);
	bool addEnemyTeam(AICoordination::teamMap& enemyTeam);
	bool addActionProcessor(ActionList* actionProcessor);
	Sensor* getSensor(SENSOR_TYPE type);
	inline AICoordination::teamMap& getTeam() {return _coordination->getTeam();}
	inline void                     setTeamID(U32 ID) {_coordination->setTeamID(ID);}
	inline U32                      getTeamID()       {return _coordination->getTeamID();}
	inline U32  getGUID() {return _GUID;}

	void sendMessage(AIEntity* receiver, AI_MSG msg,const boost::any& msg_content);
	void receiveMessage(AIEntity* sender, AI_MSG msg,const boost::any& msg_content);
	void processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content);
	const std::string& getName() {return _name;}
private:
	std::string     _name;
	U32             _GUID;
	SceneGraphNode* _node;
	AICoordination* _coordination;
	ActionList*     _actionProcessor;
	boost::mutex    _updateMutex;

	unordered_map<SENSOR_TYPE, Sensor*> _sensorList;
	
};

#endif
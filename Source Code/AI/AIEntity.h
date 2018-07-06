#ifndef _AI_ENTITY_H_
#define _AI_ENTITY_H_

#include "resource.h"
#include "Sensors/VisualSensor.h"

class SceneGraphNode;
class AIEntity{
public:
	AIEntity(const std::string& name) : _name(name) {}
	
	void processInput();
	void processData();
	void update();

	bool attachNode(SceneGraphNode* const node) {_node = node;}
private:
	std::string     _name;
	U32             _GUID;
	unordered_map<SENSOR_TYPE, Sensor*> sensorList;
	SceneGraphNode* _node;
};

#endif
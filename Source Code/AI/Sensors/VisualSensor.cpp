#include "VisualSensor.h"
#include "EngineGraphs/SceneGraphNode.h"

vec3 VisualSensor::getPositionOfObject(SceneGraphNode* node){
	return node->getTransform()->getPosition();
}
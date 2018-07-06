#include "Headers/VisualSensor.h"
#include "Graphs/Headers/SceneGraphNode.h"

vec3 VisualSensor::getPositionOfObject(SceneGraphNode* node){
	return node->getTransform()->getPosition();
}
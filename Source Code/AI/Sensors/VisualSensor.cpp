#include "Headers/VisualSensor.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

vec3<F32> VisualSensor::getPositionOfObject(SceneGraphNode* node){
	return node->getTransform()->getPosition();
}
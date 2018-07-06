#include "Headers/VisualSensor.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

using namespace AI;

vec3<F32> VisualSensor::getPositionOfObject(SceneGraphNode* node){
	return node->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
}
#include "Headers/WarSceneAIActionList.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

WarSceneAIActionList::WarSceneAIActionList() : ActionList(),
											  _node(NULL),
 											 _tickCount(0)
{
}

void WarSceneAIActionList::addEntityRef(AIEntity* entity){
	if(!entity) return;
	_entity = entity;
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		//_pozitieInitiala = visualSensor->getSpatialPosition();
	}
}

void WarSceneAIActionList::processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	switch(msg){
		case REQUEST_DISTANCE_TO_TARGET:
			updatePositions();
			break;
		case RECEIVE_DISTANCE_TO_TARGET:
			break;
	};
}

void WarSceneAIActionList::updatePositions(){
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		_tickCount++;
		if(_tickCount == 512){
			_tickCount = 0;
		}
	}
}

void WarSceneAIActionList::processInput(){
	updatePositions();
	AICoordination::teamMap& team = _entity->getTeam();
	for_each(AICoordination::teamMap::value_type& member, team){
		if(_entity->getGUID() != member.second->getGUID()){
			_entity->sendMessage(member.second, REQUEST_DISTANCE_TO_TARGET, 0);
		}
	}
}

void WarSceneAIActionList::processData(){

}

void WarSceneAIActionList::update(SceneGraphNode* node, NPC* unitRef){
	if(!_node){
		_node = node;
	}
	
	updatePositions();

	/// Updateaza informatia senzorilor
	Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);
	if(visualSensor){
		visualSensor->updatePosition(node->getTransform()->getPosition());
	}
}
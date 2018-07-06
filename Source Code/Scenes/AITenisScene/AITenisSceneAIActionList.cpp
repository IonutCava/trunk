#include "Headers/AITenisSceneAIActionList.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "AI/Sensors/Headers/VisualSensor.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

AITenisSceneAIActionList::AITenisSceneAIActionList(SceneGraphNode* target) : ActionList(),
																			 _node(NULL),
																			 _target(target),
																			 _attackBall(false),
																			 _ballToTeam2(true),
																			 _gameStop(true),
																			 _tickCount(0)
{
}

void AITenisSceneAIActionList::addEntityRef(AIEntity* entity){
	assert(entity != NULL);
	_entity = entity;
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		_initialPosition = visualSensor->getSpatialPosition();
	}
}

//Process message from sender to receiver
void AITenisSceneAIActionList::processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	AICoordination* currentTeam = NULL;
	switch(msg){
		case REQUEST_DISTANCE_TO_TARGET:
				updatePositions();
				_entity->sendMessage(sender, RECEIVE_DISTANCE_TO_TARGET, distanceToBall(_initialPosition,_ballPosition));
			break;
		case RECEIVE_DISTANCE_TO_TARGET:
			assert(_entity->getTeam());
			_entity->getTeam()->getMemberVariable()[sender] = boost::any_cast<F32>(msg_content);
			break;
		case ATTACK_BALL:
			currentTeam = _entity->getTeam();
			assert(currentTeam);
			for_each(AICoordination::teamMap::value_type const& member, currentTeam->getTeam()){
				if(_entity->getGUID() != member.second->getGUID()){
					_entity->sendMessage(member.second, DONT_ATTACK_BALL, 0);
				}
			}
			if(_ballToTeam2){
				if(_entity->getTeamID() == 2){
					_attackBall = true;
				}else{
					_attackBall = false;
				}
			}else{
				if(_entity->getTeamID() == 1){
					_attackBall = true;
				}else{
					_attackBall = false;
				}
			}
			break;
		case DONT_ATTACK_BALL: 
			_attackBall = false;
			break;

	};
}

void AITenisSceneAIActionList::updatePositions(){
	VisualSensor* visualSensor = dynamic_cast<VisualSensor*>(_entity->getSensor(VISUAL_SENSOR));
	if(visualSensor){
		_tickCount++;
		if(_tickCount == 128){
			_prevBallPosition = _ballPosition;
			_tickCount = 0;
		}
		_ballPosition = visualSensor->getPositionOfObject(_target);
		_entityPosition = visualSensor->getSpatialPosition();
		if(_prevBallPosition.z != _ballPosition.z){
			_prevBallPosition.z < _ballPosition.z ? _ballToTeam2 = false : _ballToTeam2 = true;
			_gameStop = false;
		}else{
			_gameStop = true;
		}
	}
}

///Collect all of the necessary information for this current update step
void AITenisSceneAIActionList::processInput(){
	updatePositions();
	AICoordination* currentTeam = _entity->getTeam();
	assert(currentTeam != NULL);
	_entity->getTeam()->getMemberVariable()[_entity] = distanceToBall(_initialPosition,_ballPosition);
	for_each(AICoordination::teamMap::value_type& member, currentTeam->getTeam()){
		///Ask all of our team-mates to send us their distance to the ball
		if(_entity->getGUID() != member.second->getGUID()){
			_entity->sendMessage(member.second, REQUEST_DISTANCE_TO_TARGET, 0);
		}
	}
}

void AITenisSceneAIActionList::processData(){
	AIEntity* nearestEntity = _entity;
	F32 distance = _entity->getTeam()->getMemberVariable()[_entity];
	typedef unordered_map<AIEntity*, F32 > memberVariable;
	for_each(memberVariable::value_type& member, _entity->getTeam()->getMemberVariable()){
		if(member.second < distance && member.first->getGUID() != _entity->getGUID()){
			distance = member.second;
			nearestEntity = member.first;
		}
	}
	_entity->sendMessage(nearestEntity, ATTACK_BALL, distance);
}

void AITenisSceneAIActionList::update(SceneGraphNode* node, NPC* unitRef){
	if(!unitRef)
		return;

	if(!_node){
		_node = node;
	}
	
	updatePositions();

	if(_attackBall && !_gameStop){
		/// Try to intercept the ball
		unitRef->moveToX(_ballPosition.x);
	}else{
		/// Try to return to original position
		unitRef->moveToX(_initialPosition.x);
	}

	/// Update sensor information
	Sensor* visualSensor = _entity->getSensor(VISUAL_SENSOR);
	if(visualSensor){
		visualSensor->updatePosition(node->getTransform()->getPosition());
	}
}


///Only X axis absolute value is important
F32 AITenisSceneAIActionList::distanceToBall(const vec3<F32>& entityPosition, const vec3<F32> ballPosition) {
	return abs(ballPosition.x - entityPosition.x);
}
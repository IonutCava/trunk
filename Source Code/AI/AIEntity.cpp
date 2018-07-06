#include "Headers/AIEntity.h"
#include "ActionInterface/Headers/ActionList.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "PathFinding/Headers/WaypointGraph.h"  ///< For waypoint movement
#include "PathFinding/Headers/NavigationMesh.h" ///< For NavMesh movement

AIEntity::AIEntity(const std::string& name) : _name(name),
											  _actionProcessor(NULL),
											  _unitRef(NULL),
											  _coordination(NULL)
{
	_GUID = GETMSTIME() * random(55);
}

void AIEntity::sendMessage(AIEntity* receiver, AI_MSG msg,const boost::any& msg_content){
	CommunicationSensor* com = dynamic_cast<CommunicationSensor*>(getSensor(COMMUNICATION_SENSOR));
	com->sendMessageToEntity(receiver, msg,msg_content);
}

void AIEntity::receiveMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	CommunicationSensor* com = dynamic_cast<CommunicationSensor*>(getSensor(COMMUNICATION_SENSOR));
	com->receiveMessageFromEntity(sender, msg,msg_content);
}

void AIEntity::processMessage(AIEntity* sender, AI_MSG msg, const boost::any& msg_content) {
	_actionProcessor->processMessage(sender, msg, msg_content);
}

Sensor* AIEntity::getSensor(SENSOR_TYPE type){
	if(_sensorList.find(type) != _sensorList.end()){
		return _sensorList[type];
	}
	return NULL;
}

bool AIEntity::addSensor(SENSOR_TYPE type, Sensor* sensor){
	if(_sensorList.find(type) != _sensorList.end()){
		delete _sensorList[type];
	}
	sensor->updatePosition(_node->getTransform()->getPosition());
	_sensorList[type] = sensor;
	return true;
}


bool AIEntity::addActionProcessor(ActionList* actionProcessor){
	if(_actionProcessor){
		_updateMutex.lock();
		delete _actionProcessor;
		_updateMutex.unlock();
	}
	_updateMutex.lock();
	_actionProcessor = actionProcessor;
	_actionProcessor->addEntityRef(this);
	_updateMutex.unlock();
	return true;
}

void AIEntity::processInput(){
	if(!_actionProcessor) return;
	_actionProcessor->processInput();
}

void AIEntity::processData(){
	if(!_actionProcessor) return;
	_actionProcessor->processData();
}

void AIEntity::update(){
	if(!_actionProcessor) return;
	_updateMutex.lock();
	_actionProcessor->update(_node, _unitRef);
	_updateMutex.unlock();
}


void AIEntity::setTeam(AICoordination* const coordination) {
	if(_coordination){
		///Remove from old team
		_coordination->removeTeamMember(this);
	}
	///Update our team
	_coordination = coordination;
	///Add ourself to the new team
	_coordination->addTeamMember(this);
}

bool AIEntity::addFriend(AIEntity* const friendEntity){
	AICoordination* friendTeam = friendEntity->getTeam();
	///If no team, check if our friend has a team and add ourself to it
	if(!_coordination){
		///If our friend has a team ...
		if(friendTeam){
			///Create friendship
			friendTeam->addTeamMember(this);
			_coordination = friendTeam;
			return true;
		}
		return false;
	}
	///If we have team, add friend to our team
	_coordination->addTeamMember(friendEntity);
	///If our friend isn't on our team, add him
	if(!friendTeam){
		friendEntity->setTeam(_coordination);
	}
	return true;
}
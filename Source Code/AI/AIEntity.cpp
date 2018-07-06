#include "Headers/AIEntity.h"
#include "ActionInterface/Headers/ActionList.h"

#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "PathFinding/Headers/WaypointGraph.h"  ///< For waypoint movement
#include "PathFinding/Headers/NavigationMesh.h" ///< For NavMesh movement

AIEntity::AIEntity(const std::string& name)  : GUIDWrapper(),
                                              _name(name),
											  _actionProcessor(NULL),
											  _unitRef(NULL),
											  _coordination(NULL),
											  _comInterface(NULL)
{
}

void AIEntity::sendMessage(AIEntity* receiver, AIMsg msg,const boost::any& msg_content){
	CommunicationInterface* com = getCommunicationInterface();
	if(com){
		com->sendMessageToEntity(receiver, msg,msg_content);
	}
}

void AIEntity::receiveMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content){
	CommunicationInterface* com = getCommunicationInterface();
	if(com){
		com->receiveMessageFromEntity(sender, msg,msg_content);
	}
}

void AIEntity::processMessage(AIEntity* sender, AIMsg msg, const boost::any& msg_content) {
	ReadLock r_lock(_updateMutex);
	_actionProcessor->processMessage(sender, msg, msg_content);
}

Sensor* AIEntity::getSensor(SensorType type){
	if(_sensorList.find(type) != _sensorList.end()){
		return _sensorList[type];
	}
	return NULL;
}

bool AIEntity::addSensor(SensorType type, Sensor* sensor){
	sensor->updatePosition(_node->getTransform()->getPosition());
	if(_sensorList.find(type) != _sensorList.end()){
		SAFE_UPDATE(_sensorList[type], sensor);
	}else{
		_sensorList.insert(std::make_pair(type,sensor));
	}
	return true;
}

bool AIEntity::addActionProcessor(ActionList* actionProcessor){
	WriteLock w_lock(_updateMutex);
	SAFE_UPDATE(_actionProcessor, actionProcessor);
	_actionProcessor->addEntityRef(this);
	return true;
}

void AIEntity::processInput(){
	ReadLock r_lock(_managerQueryMutex);
	if(!_actionProcessor) return;
	_actionProcessor->processInput();
}

void AIEntity::processData(){
	ReadLock r_lock(_managerQueryMutex);
	if(!_actionProcessor) return;
	_actionProcessor->processData();
}

void AIEntity::update(){
	ReadLock r_lock(_managerQueryMutex);
	if(!_actionProcessor) return;
	_actionProcessor->update(_node, _unitRef);
}

void AIEntity::setTeam(AICoordination* const coordination) {
	ReadLock r_lock(_updateMutex);
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
	ReadLock r_lock(_updateMutex);
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
#include "AIEntity.h"
#include "ActionInterface/ActionList.h"
#include "EngineGraphs/SceneGraphNode.h"

AIEntity::AIEntity(const std::string& name) : _name(name),
											  _coordination(New AICoordination()),
											  _actionProcessor(NULL) {
	_GUID = GETMSTIME() * random(55);
	_coordination->addTeamMember(this);
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

bool AIEntity::addFriend(AIEntity* entity){
	bool state = false;
	state = _coordination->addTeamMember(entity);
	if(state){
		state = entity->_coordination->addTeamMember(this);
	}

	return state;
}

bool AIEntity::addEnemyTeam(AICoordination::teamMap& enemyTeam){
	for_each(AICoordination::teamMap::value_type& member, _coordination->getTeam()){
		if(!member.second->_coordination->addEnemyTeam(enemyTeam)){
			Console::getInstance().errorfn("AI: Error adding enemy team to member [ %s ]", member.second->_name.c_str());
			return false;
		}
	}
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
	_actionProcessor->update(_node);
	_updateMutex.unlock();
}


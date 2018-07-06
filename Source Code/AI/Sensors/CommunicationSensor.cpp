#include "Headers/CommunicationSensor.h"
#include "AI/Headers/AIEntity.h"

bool CommunicationSensor::sendMessageToEntity(AIEntity* receiver, AI_MSG msg, const boost::any& msg_content){
	if(receiver){
		receiver->receiveMessage(_entity, msg, msg_content);
	}
	return true;
}

bool CommunicationSensor::receiveMessageFromEntity(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	if(_entity){
		_entity->processMessage(sender, msg, msg_content);
	}
	return true;
}
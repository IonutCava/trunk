#include "Headers/CommunicationInterface.h"
#include "AI/Headers/AIEntity.h"

bool CommunicationInterface::sendMessageToEntity(AIEntity* receiver, AI_MSG msg, const boost::any& msg_content){
	if(receiver){
		receiver->receiveMessage(_entity, msg, msg_content);
	}
	return true;
}

bool CommunicationInterface::receiveMessageFromEntity(AIEntity* sender, AI_MSG msg, const boost::any& msg_content){
	if(_entity){
		_entity->processMessage(sender, msg, msg_content);
	}
	return true;
}
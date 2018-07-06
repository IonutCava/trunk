#ifndef _AI_AUDIO_SENSOR_H_
#define _AI_AUDIO_SENSOR_H_

#include "Sensor.h"

enum AI_MSG;
class AIEntity;
class CommunicationSensor : public Sensor{

public:
	CommunicationSensor(AIEntity* entity) : _entity(entity), Sensor(COMMUNICATION_SENSOR) {}

	bool sendMessageToEntity(AIEntity* receiver, AI_MSG msg,const boost::any& msg_content);
	bool receiveMessageFromEntity(AIEntity* sender, AI_MSG msg, const boost::any& msg_content);

private:
	AIEntity* _entity;
};

#endif
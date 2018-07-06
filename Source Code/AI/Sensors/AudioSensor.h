#ifndef _AI_AUDIO_SENSOR_H_
#define _AI_AUDIO_SENSOR_H_

#include "Sensor.h"

class AIEntity;
class AudioSensor : public Sensor{
public: 
	AudioSensor() : Sensor(AUDIO_SENSOR) {}

	U32  getNearbyEntityCount(U32 range)         {}  //number of smart units nearby     (only noisy ones)
	U32  getNearbyHostileEntityCount(U32 range)  {}  //lookup hostile entity count in range (only noisy ones)
	U32  getNearbyFriendlyEntityCount(U32 range) {}  //lookup friendly entity count in range (only noisy ones)

	AIEntity* getNearestFriendlyEntity()         {}  //get closest noisy friendly entity
	AIEntity* getNearestHostileEntity()          {}  //get closest noisy hostile entity

};

#endif
#ifndef _AI_VISUAL_SENSOR_H_
#define _AI_VISUAL_SENSOR_H_

#include "Sensor.h"

class AIEntity;
class VisualSensor : public Sensor{
public: 
	VisualSensor() : Sensor(VISUAL_SENSOR) {}

	U32  getNearbyEntityCount(U32 range)         {}  //number of smart units nearby     (only visible ones)
	U32  getNearbyHostileEntityCount(U32 range)  {}  //lookup hostile entity count in range (only visible ones)
	U32  getNearbyFriendlyEntityCount(U32 range) {}  //lookup friendly entity count in range (only visible ones)
	vec2 getSpatialPosition()                    {}  //return the coordinates at which the sensor is found 
													 //(or the entity it's attached to)
	U32  getDistanceToEntity(AIEntity* target)   {}
	AIEntity* getNearestFriendlyEntity()         {}  //get closest visible friendly entity
	AIEntity* getNearestHostileEntity()          {}  //get closest visible hostile entity

};

#endif
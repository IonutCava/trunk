#ifndef _AI_SENSOR_H_
#define _AI_SENSOR_H_
#include "resource.h"
#include <boost/any.hpp>

enum SENSOR_TYPE{
	NONE = 0,
	VISUAL_SENSOR = 1,
	AUDIO_SENSOR = 2,
	COMMUNICATION_SENSOR = 3
};

class Sensor{
public:
	Sensor(SENSOR_TYPE type){_type = type;}
	virtual void updatePosition(const vec3& newPosition) {_position = newPosition;}
	vec3& getSpatialPosition()                    {return _position;}  //return the coordinates at which the sensor is found 
												                      //(or the entity it's attached to)
	SENSOR_TYPE getSensorType() {return _type;}
	
protected:

	vec3 _position;
	vec2 _range; //min/max
	SENSOR_TYPE _type;
};

#endif 
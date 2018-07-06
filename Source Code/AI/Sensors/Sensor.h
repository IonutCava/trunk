#ifndef _AI_SENSOR_H_
#define _AI_SENSOR_H_
#include "resource.h"

enum SENSOR_TYPE{
	NONE = 0,
	VISUAL_SENSOR = 1,
	AUDIO_SENSOR = 2
};

class Sensor{
public:
	Sensor(SENSOR_TYPE type){_type = type;}
	void updatePosition(const vec2& newPosition) {_position = newPosition;}
	vec2 getSpatialPosition()                    {return _position;}  //return the coordinates at which the sensor is found 
												                      //(or the entity it's attached to)
	SENSOR_TYPE getSensorType() {return _type;}
private:

	vec2 _position;
	vec2 _range; //min/max
	SENSOR_TYPE _type;
};

#endif 
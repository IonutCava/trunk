/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _AI_SENSOR_H_
#define _AI_SENSOR_H_

#include "core.h"

enum SensorType{
	NONE = 0,
	VISUAL_SENSOR = 1,
	AUDIO_SENSOR = 2
};

class Sensor{
public:
	Sensor(SensorType type){_type = type;}
	virtual void updatePosition(const vec3<F32>& newPosition) {_position = newPosition;}
	/// return the coordinates at which the sensor is found (or the entity it's attached to)
	inline vec3<F32>& getSpatialPosition() {return _position;}  
	inline SensorType getSensorType()     {return _type;}
	
protected:

	vec3<F32> _position;
	vec2<F32> _range; ///< min/max
	SensorType _type;
};

#endif 
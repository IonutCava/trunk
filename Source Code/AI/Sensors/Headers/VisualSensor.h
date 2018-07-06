/*“Copyright 2009-2012 DIVIDE-Studio”*/
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

#ifndef _AI_VISUAL_SENSOR_H_
#define _AI_VISUAL_SENSOR_H_

#include "Sensor.h"

class AIEntity;
class SceneGraphNode;
class VisualSensor : public Sensor{

public: 
	VisualSensor() : Sensor(VISUAL_SENSOR) {}

	U32  getNearbyEntityCount(U32 range)         {}  ///< number of smart units nearby     (only visible ones)
	U32  getNearbyHostileEntityCount(U32 range)  {}  ///< lookup hostile entity count in range (only visible ones)
	U32  getNearbyFriendlyEntityCount(U32 range) {}  ///< lookup friendly entity count in range (only visible ones)

	U32        getDistanceToEntity(AIEntity* target)   {}
	vec3<F32>  getPositionOfObject(SceneGraphNode* node);    
	AIEntity*  getNearestFriendlyEntity()         {}  ///< get closest visible friendly entity
	AIEntity*  getNearestHostileEntity()          {}  ///< get closest visible hostile entity
};

#endif
/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _AI_VISUAL_SENSOR_H_
#define _AI_VISUAL_SENSOR_H_

#include "Sensor.h"

class SceneGraphNode;
namespace AI {
class AIEntity;

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
}; //namespace AI
#endif
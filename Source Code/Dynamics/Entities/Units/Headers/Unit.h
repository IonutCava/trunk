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

#ifndef _UNIT_H_
#define _UNIT_H_

#include "resource.h"

class SceneGraphNode;
///Unit interface
class Unit {
public:
	/// Currently supported unit types
	enum UNIT_TYPE{
		/// "Living beings"
		UNIT_TYPE_CHARACTER,
		/// e.g. Cars, planes, ships etc
		UNIT_TYPE_VEHICLE,
		/// add more types above this
		UNIT_TYPE_PLACEHOLDER
	};

	Unit(UNIT_TYPE type, SceneGraphNode* const node);
	~Unit();

	/// moveTo makes the unit follow a path from it's current position to the targets position
	virtual bool moveTo(const vec3& targetPosition);
	/// teleportTo instantly places the unit at the desired position
	virtual bool teleportTo(const vec3& targetPosition);

	/// Accesors
	/// Get the units position in the world
	inline const vec3& getCurrentPosition() {return _currentPosition;}
	/// Get the units target coordinates in the world
	inline const vec3& getTargetPosition() {return _currentTargetPosition;}
	/// Set the units movement speed (minimum is 0 = the unit does not move / is rooted)
	inline void setMovementSpeed(F32 movementSpeed) {_moveSpeed = Util::min(0, movementSpeed);}
	/// Get the units current movement speed
	inline F32  getMovementSpeed()                  {return _moveSpeed;}
	/// Set unit type
	inline void setUnitType(UNIT_TYPE type) {_type = type;}
	/// Get unit type
	inline UNIT_TYPE getUnitType()          {return _type;}

private:
	/// Unit type
	UNIT_TYPE _type;
	/// Movement speed
	F32 _moveSpeed;
	/// Unit position in world
	vec3  _currentPosition;
	/// Current destination point cached
	vec3 _currentTargetPosition;
	/// SceneGraphNode the unit is managing (used for updating positions and checking collisions
	SceneGraphNode* _node;
};

#endif
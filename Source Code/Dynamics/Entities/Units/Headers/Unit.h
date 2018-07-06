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

#ifndef _UNIT_H_
#define _UNIT_H_

#include "core.h"

class SceneGraphNode;
///Unit interface
class Unit {
public:
	/// Currently supported unit types
	enum UnitType{
		/// "Living beings"
		UNIT_TYPE_CHARACTER,
		/// e.g. Cars, planes, ships etc
		UNIT_TYPE_VEHICLE,
		/// add more types above this
		UNIT_TYPE_PLACEHOLDER
	};

	Unit(UnitType type, SceneGraphNode* const node);
	~Unit();

	/// moveTo makes the unit follow a path from it's current position to the targets position
	virtual bool moveTo(const vec3<F32>& targetPosition);
	virtual bool moveToX(const F32 targetPosition);
	virtual bool moveToY(const F32 targetPosition);
	virtual bool moveToZ(const F32 targetPosition);
	/// teleportTo instantly places the unit at the desired position
	virtual bool teleportTo(const vec3<F32>& targetPosition);

	/// Accesors
	/// Get the units position in the world
	inline const vec3<F32>& getCurrentPosition() {return _currentPosition;}
	/// Get the units target coordinates in the world
	inline const vec3<F32>& getTargetPosition() {return _currentTargetPosition;}
	/// Set the units movement speed (minimum is 0 = the unit does not move / is rooted)
	inline void setMovementSpeed(F32 movementSpeed) {_moveSpeed = movementSpeed; CLAMP<F32>(_moveSpeed,0.0f,100.0f);}
	/// Get the units current movement speed
	inline F32  getMovementSpeed()                  {return _moveSpeed;}
	/// Set destination tolerance
	inline void setMovementTolerance(F32 movementTolerance) {_moveTolerance = Util::max(0.1f,movementTolerance);}
	/// Get the units current movement tolerance
	inline F32  getMovementTolerance()                      {return _moveTolerance;}
	/// Set unit type
	inline void setUnitType(UnitType type) {_type = type;}
	/// Get unit type
	inline UnitType getUnitType()          {return _type;}

private:
	/// Unit type
	UnitType _type;
	/// Movement speed (per second)
	F32 _moveSpeed;
	/// acceptable distance from target
	F32 _moveTolerance;
	/// previous time, in milliseconds when last move was applied
	U32 _prevTime;
	/// Unit position in world
	vec3<F32>  _currentPosition;
	/// Current destination point cached
	vec3<F32> _currentTargetPosition;
	/// SceneGraphNode the unit is managing (used for updating positions and checking collisions
	SceneGraphNode* _node;
	mutable SharedLock _unitUpdateMutex;
};

#endif
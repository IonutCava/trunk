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

#ifndef _VEHICLE_H_
#define _VEHICLE_H_

#include "Unit.h"

/// Basic vehicle class
class Vehicle : public Unit {
public:
	/// Currently supported vehicle types
	enum VEHICLE_TYPES{
		/// Ground based vehicles
		VEHICLE_TYPE_GROUND      = toBit(1),
		/// Flying/Space vehicles
		VEHICLE_TYPE_AIR         = toBit(2),
		/// Water based vehicles (ships, boats, etc)
		VEHICLE_TYPE_WATER       = toBit(3),
		/// Underwater vehicles
		VEHICLE_TYPE_UNDERWATER  = toBit(4),
		/// For Future expansion
		VEHICLE_TYPE_PLACEHOLDER = toBit(10)
	};

	Vehicle(SceneGraphNode* const node);
	~Vehicle();

	/// A vehicle can be of multiple types at once
	void setVehicleTypeMask(U8 mask);
	/// Check if current vehicle fits the desired type
	bool checkVehicleMask(VEHICLE_TYPES type) const;

public:
	/// Is this vehicle controlled by the player or the AI?
	bool _playerControlled;
	U8   _vehicleTypeMask;
};

#endif
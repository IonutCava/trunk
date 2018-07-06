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

#ifndef _WAYPOINT_H_
#define _WAYPOINT_H_
#include "resource.h"
#include "Core/Math/Headers/Ray.h"
#include "Core/Math/Headers/Quaternion.h"

namespace Navigation {
	/// A point in space that AI units can navigate to
	class Waypoint {
	public:
		Waypoint();
		~Waypoint();

		inline U32 getID() const {return _id;}

	public: //ToDo: add accessors 
		vec3<F32>  _position;
		Quaternion _orientation;
		U32        _time;

	private:
		U32 _id;
	};

	/// A straight line between 2 waypoints
	///
	class WaypointPath {
	public:
		WaypointPath(Waypoint* first, Waypoint* second);
		~WaypointPath();

	private:
		Waypoint* _first;
		Waypoint* _second;
		/// If the path intersects an object in the scene, is the path still valid?
		bool _throughObjects;
		/// ray used for collision detection
		Ray _collisionRay;
	};
};

#endif
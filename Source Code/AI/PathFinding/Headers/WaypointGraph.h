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
#ifndef _WAYPOINT_GRAPH_H_
#define _WAYPOINT_GRAPH_H_
#include "Waypoint.h"
namespace Navigation {
	class WaypointGraph {
		typedef unordered_map<I32, Waypoint*> WaypointMap; 
		//typedef unordered_map<I32, WaypointPath> PathMap;
	public:
		WaypointGraph();
		~WaypointGraph();

	   void addWaypoint(Waypoint* wp);
	   void removeWaypoint(Waypoint* wp);

	   void sortMarkers();
	   void updateGraph();
	   bool isLooping() { return _loop; }
	   inline U32 getID() const {return _id;}
	   inline U32 getSize() {return _waypoints.size();}

	private:
		WaypointMap _waypoints;
		//PathMap     _paths;

		U32 _id;
		bool _loop;

		std::vector<vec3<F32> >         _positions;
		std::vector<Quaternion<F32> >   _rotations;
		std::vector<U32>                _times;
	};
};


#endif
#include "Headers/WaypointGraph.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace Navigation {
	WaypointGraph::WaypointGraph(){
		_id = 0xFFFFFFFF;
		_loop = true;
	}

	WaypointGraph::~WaypointGraph(){
		_waypoints.clear();
	}

	void WaypointGraph::addWaypoint(Waypoint* wp){
		if(_waypoints.find(wp->getID()) != _waypoints.end()) return;
		hashAlg::emplace(_waypoints, wp->getID(), wp);
		updateGraph();
	}

	void WaypointGraph::removeWaypoint(Waypoint* wp){
		if(_waypoints.find(wp->getID()) != _waypoints.end()){
			_waypoints.erase(wp->getID());
			updateGraph();
		}else{
			PRINT_FN(Locale::get("WARN_WAYPOINT_NOT_FOUND"),wp->getID(), getID());
		}
	}

	void WaypointGraph::updateGraph(){
	   typedef hashMapImpl<U32, Waypoint*> wp;
	   _positions.clear();
	   _rotations.clear();
	   _times.clear();
	   for (wp::value_type& waypoint : _waypoints){
		  _positions.push_back((waypoint.second)->_position);
		  _rotations.push_back((waypoint.second)->_orientation);
		  _times.push_back((waypoint.second)->_time);
	   }
	}
};
};
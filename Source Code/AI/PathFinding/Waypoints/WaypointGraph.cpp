#include "stdafx.h"

#include "Headers/WaypointGraph.h"

#include "Core/Headers/Console.h"
#include "Utility/Headers/Localization.h"

namespace Divide {
namespace Navigation {
WaypointGraph::WaypointGraph() {
    _id = 0xFFFFFFFF;
    _loop = true;
}

WaypointGraph::~WaypointGraph() { _waypoints.clear(); }

void WaypointGraph::addWaypoint(Waypoint* wp) {
    if (_waypoints.find(wp->ID()) != std::end(_waypoints)) {
        return;
    }

    hashAlg::insert(_waypoints, wp->ID(), wp);
    updateGraph();
}

void WaypointGraph::removeWaypoint(Waypoint* wp) {
    if (_waypoints.find(wp->ID()) != std::end(_waypoints)) {
        _waypoints.erase(wp->ID());
        updateGraph();
    } else {
        Console::printfn(Locale::get(_ID("WARN_WAYPOINT_NOT_FOUND")), wp->ID(),
                         getID());
    }
}

void WaypointGraph::updateGraph() {
    typedef hashMap<U32, Waypoint*> wp;
    _positions.resize(0);
    _rotations.resize(0);
    _times.resize(0);
    for (wp::value_type& waypoint : _waypoints) {
        _positions.push_back((waypoint.second)->position());
        _rotations.push_back((waypoint.second)->orientation());
        _times.push_back((waypoint.second)->time());
    }
}
};
};
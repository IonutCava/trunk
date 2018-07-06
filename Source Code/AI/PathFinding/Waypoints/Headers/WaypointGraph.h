/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _WAYPOINT_GRAPH_H_
#define _WAYPOINT_GRAPH_H_
#include "Waypoint.h"

namespace Divide {
    namespace Navigation {

    class WaypointGraph {
        typedef hashMapImpl<U32, Waypoint*> WaypointMap;
        //typedef hashMapImpl<I32, WaypointPath> PathMap;
    public:
        WaypointGraph();
        ~WaypointGraph();

       void addWaypoint(Waypoint* wp);
       void removeWaypoint(Waypoint* wp);

       void sortMarkers();
       void updateGraph();
       bool isLooping() { return _loop; }
       inline U32 getID() const {return _id;}
       inline U32 getSize() {return (U32)_waypoints.size();}

    private:
        WaypointMap _waypoints;
        //PathMap     _paths;

        U32  _id;
        bool _loop;

        vectorImpl<vec3<F32> >         _positions;
        vectorImpl<Quaternion<F32> >   _rotations;
        vectorImpl<U32>                _times;
    };

    }; //namespace Navigation
}; //namespace Divide

#endif
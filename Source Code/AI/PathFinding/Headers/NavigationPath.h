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

//
// T3D 1.1 NavPath class for use with NavMesh, based on the Recast/Detour library.
// Daniel Buckmaster, 2011
//

#ifndef _NAVIGATION_PATH_H_
#define _NAVIGATION_PATH_H_

#include "core.h" 
#include "NavigationMesh.h"
#include "AI/PathFinding/Detour/Headers/DetourNavMeshQuery.h"

namespace Navigation {

   static const U32 MaxPathLen = 1024;

   class tQueryFilter : public dtQueryFilter {
      typedef dtQueryFilter Parent;

   public:
      /// Default constructor.
      tQueryFilter();

      bool passFilter(const dtPolyRef ref, const dtMeshTile* tile, const dtPoly* poly) const;

   };

   class WaypointGraph;   
   class NavigationPath/*: public SceneObject */{
      //typedef SceneObject Parent;
   public:
      /// @name NavigationPath
      /// Functions for planning and accessing the path.
      /// @{

      NavigationMesh *_mesh;
      WaypointGraph *_waypoints;

      vec3<F32> _from;
      bool      _fromSet;
      vec3<F32> _to;
      bool      _toSet;

      bool _isLooping;
      bool _autoUpdate;
      bool _isSliced;

      I32 _maxIterations;

      bool _alwaysRender;
      bool _Xray;

      /// Plan the path.
      bool plan();

      /// Updated a sliced plan.
      /// @return True if we need to keep updating, false if we can stop.
      bool update();

      /// Finalise a sliced plan.
      /// @return True if the plan was successful overall.
      bool finalise();

      /// Return world-space position of a path node.
      vec3<F32> getNode(I32 idx);

      /// @}


      /// Return the number of nodes in this path.
      I32 getCount();

      /// Return our own ID number, and set internal logic to report our
      /// position as being the sub-position represented by idx.
      /// @param[in] idx Path point.
      /// @return Our own id.
      I32 getObject(I32 idx);

      /// Return the length of this path.
      F32 getLength() { return _length; };

 
      /// @name ProcessObject
      /// @{
      /// void processTick(const Move *move);
      /// @}

      NavigationPath();
      ~NavigationPath();

   private:
      /// Create appropriate data structures and stuff.
      bool init();

      /// Plan the path.
      bool planInstant();

      /// Start a sliced plan.
      /// @return True if the plan initialised successfully.
      bool planSliced();

      /// Add points of the path between the two specified points.
      //bool addPoints(Point3F from, Point3F to, Vector<Point3F> *points);

      /// 'Visit' the last two points on our visit list.
      bool visitNext();

      dtNavMeshQuery*         _query;
      dtStatus                _status;
      tQueryFilter            _filter;
      I32                     _curIndex;
	  std::vector<vec3<F32> > _points;
      std::vector<vec3<F32> > _visitPoints;
      F32					  _length;

      /// Resets our world transform and bounds to fit our point list.
      void resize();

   };
};

#endif 
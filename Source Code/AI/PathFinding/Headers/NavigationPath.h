/*
   Copyright (c) 2013 DIVIDE-Studio
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
//
// T3D 1.1 NavPath class for use with NavMesh, based on the Recast/Detour library.
// Daniel Buckmaster, 2011
//

#ifndef _NAVIGATION_PATH_H_
#define _NAVIGATION_PATH_H_

#include "core.h"
#include "NavMeshes/Headers/NavMesh.h"

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
      //bool addPoints(Point3F from, Point3F to, vectorImpl<Point3F> *points);

      /// 'Visit' the last two points on our visit list.
      bool visitNext();

      dtNavMeshQuery*         _query;
      dtStatus                _status;
      tQueryFilter            _filter;
      I32                     _curIndex;
      vectorImpl<vec3<F32> > _points;
      vectorImpl<vec3<F32> > _visitPoints;
      F32					  _length;

      /// Resets our world transform and bounds to fit our point list.
      void resize();
   };
};

#endif 
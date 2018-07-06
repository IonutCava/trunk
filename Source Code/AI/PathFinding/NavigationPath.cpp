#include "Headers/NavigationPath.h"
#include "Waypoints/Headers/WaypointGraph.h"

namespace Navigation {
   tQueryFilter::tQueryFilter() {
   }

   bool tQueryFilter::passFilter(const dtPolyRef ref, const dtMeshTile* tile, const dtPoly* poly) const  {
      return Parent::passFilter(ref, tile, poly);
   }

   NavigationPath::NavigationPath() :  _from(0.0f, 0.0f, 0.0f),
                                       _to(0.0f, 0.0f, 0.0f)  {
      _mesh = NULL;
      _waypoints = NULL;

      _from.set(0, 0, 0);
      _fromSet = false;
      _to.set(0, 0, 0);
      _toSet = false;
      _length = 0.0f;

      _curIndex = -1;
      _isLooping = false;
      _autoUpdate = false;
      _isSliced = false;

      _maxIterations = 1;

      _alwaysRender = false;
      _Xray = false;

      _query = dtAllocNavMeshQuery();
   }

   NavigationPath::~NavigationPath() {
      dtFreeNavMeshQuery(_query);
      _query = NULL;
   }

  bool NavigationPath::init() {
      // Check that al the right data is provided.
      if(!_mesh || !_mesh->getNavigationMesh())
         return false;

      if(!(_fromSet && _toSet) && !(_waypoints && _waypoints->getSize()))
         return false;

      // Initialise out query.
      if(dtStatusFailed(_query->init(_mesh->getNavigationMesh(), MaxPathLen)))
         return false;

      _points.clear();
      _visitPoints.clear();
      _length = 0.0f;

      return true;
   }

   void NavigationPath::resize()  {
   }

   bool NavigationPath::plan()  {
      if(_isSliced)
         return planSliced();
      else
         return planInstant();
   }

   bool NavigationPath::planSliced()  {
      // Initialise query and visit locations.
      if(!init())
         return false;

      bool visited = visitNext();

      return visited;
   }

   bool NavigationPath::visitNext()  {
      U32 s = _visitPoints.size();

      if(s < 2)
         return false;

      // Current leg of journey.
      vec3<F32> start = _visitPoints[s-1];
      vec3<F32> end   = _visitPoints[s-2];

      // Convert to Detour-friendly coordinates and data structures.
      F32 from[] = {start.x, start.z, -start.y};
      F32 to[] =   {end.x,   end.z,   -end.y};
      F32 extents[] = {1.0f, 1.0f, 1.0f};
      dtPolyRef startRef, endRef;

      if(dtStatusFailed(_query->findNearestPoly(from, extents, &_filter, &startRef, start)))  {
          ERROR_FN(Locale::get("ERROR_NAV_NO_POLY_NEAR_POINTS"), start.x, start.y, start.z);
         return false;
      }

      if(dtStatusFailed(_query->findNearestPoly(to, extents, &_filter, &endRef, end)))
      {
          ERROR_FN(Locale::get("ERROR_NAV_NO_POLY_NEAR_POINTS"), end.x, end.y, end.z);
         return false;
      }

      // Init sliced pathfind.
      _status = _query->initSlicedFindPath(startRef, endRef, from, to, &_filter);

      if(dtStatusFailed(_status))
         return false;

      return true;
   }

   bool NavigationPath::update()  {
      return false;
   }

   bool NavigationPath::finalise()  {
      resize();

      return dtStatusSucceed(_status);
   }

   bool NavigationPath::planInstant()  {
      if(!init())
         return false;

      visitNext();

      while(update());

      if(!finalise())
         return false;

      resize();

      return true;
   }

   vec3<F32> NavigationPath::getNode(I32 idx)  {
      if(idx < getCount() && idx >= 0)
         return _points[idx];

      return vec3<F32>();
   }

   I32 NavigationPath::getCount()  {
      return _points.size();
   }

   I32 NavigationPath::getObject(I32 idx)  {
      return -1;
   }
};
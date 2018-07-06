#include "Headers/DivideRecast.h"
#include "Waypoints/Headers/WaypointGraph.h"

namespace Navigation {
    static const U8  DT_TILECACHE_NULL_AREA     = 0;
    static const U8  DT_TILECACHE_WALKABLE_AREA = 63;
    static const U16 DT_TILECACHE_NULL_IDX      = 0xffff;

    DivideRecast::DivideRecast()
    {
        // Setup the default query filter
        _filter = new dtQueryFilter();
        _filter->setIncludeFlags(0xFFFF);    // Include all
        _filter->setExcludeFlags(0);         // Exclude none
        // Area flags for polys to consider in search, and their cost
        _filter->setAreaCost(SAMPLE_POLYAREA_GROUND, 1.0f);       // TODO have a way of configuring the filter
        _filter->setAreaCost(DT_TILECACHE_WALKABLE_AREA, 1.0f);
        
        // Init path store. MaxVertex 0 means empty path slot
        for(I32 i = 0; i < MAX_PATHSLOT; i++) {
            _pathStore[i].MaxVertex = 0;
            _pathStore[i].Target = 0;
        }
    }

    DivideRecast::~DivideRecast()
    {
        SAFE_DELETE(_filter);
    }

    PathErrorCode DivideRecast::FindPath(const NavigationMesh& navMesh, 
                                         const vec3<F32>& startPos, 
                                         const vec3<F32>& endPos, 
                                         I32 pathSlot,
                                         I32 target) {
        
        const F32* pStartPos = &startPos[0];
        const F32* pEndPos = &endPos[0];
        const F32* extents = &navMesh.getExtents()[0];
        const dtNavMeshQuery& navQuery = navMesh.getNavQuery();

        dtStatus status;
        dtPolyRef StartPoly;
        dtPolyRef EndPoly;
        dtPolyRef PolyPath[MAX_PATHPOLY];
        F32 StraightPath[MAX_PATHVERT * 3];
        F32 StartNearest[3];
        F32 EndNearest[3];
        I32 nPathCount = 0;
        I32 nVertCount=0;

        // find the start polygon
        status = navQuery.findNearestPoly(pStartPos, extents, _filter, &StartPoly, StartNearest) ;
        if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK)) 
            return PATH_ERROR_NO_NEAREST_POLY_START; // couldn't find a polygon

        // find the end polygon
        status = navQuery.findNearestPoly(pEndPos, extents, _filter, &EndPoly, EndNearest) ;
        if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
            return PATH_ERROR_NO_NEAREST_POLY_END; // couldn't find a polygon

        status = navQuery.findPath(StartPoly, EndPoly, StartNearest, EndNearest, _filter, PolyPath, &nPathCount, MAX_PATHPOLY) ;
        if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
            return PATH_ERROR_COULD_NOT_CREATE_PATH; // couldn't create a path
        if(nPathCount==0)
            return PATH_ERROR_COULD_NOT_FIND_PATH; // couldn't find a path

        status = navQuery.findStraightPath(StartNearest, EndNearest, PolyPath, nPathCount, StraightPath, nullptr, nullptr, &nVertCount, MAX_PATHVERT) ;
        if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
            return PATH_ERROR_NO_STRAIGHT_PATH_CREATE; // couldn't create a path
        if(nVertCount==0)
            return PATH_ERROR_NO_STRAIGHT_PATH_FIND; // couldn't find a path

        // At this point we have our path.  Copy it to the path store
        I32 nIndex=0 ;
        for(I32 nVert = 0; nVert < nVertCount; nVert++) {
            _pathStore[pathSlot].PosX[nVert]=StraightPath[nIndex++];
            _pathStore[pathSlot].PosY[nVert]=StraightPath[nIndex++];
            _pathStore[pathSlot].PosZ[nVert]=StraightPath[nIndex++];

           //PRINT_FN("Path Vert %i, %f %f %f", nVert, m_PathStore[pathSlot].PosX[nVert], m_PathStore[pathSlot].PosY[nVert], m_PathStore[pathSlot].PosZ[nVert]) ;
        }
   
        _pathStore[pathSlot].MaxVertex=nVertCount;
        _pathStore[pathSlot].Target=target;

        return PATH_ERROR_NONE;
    }

    vectorImpl<vec3<F32> > DivideRecast::getPath(I32 pathSlot) {
       
        vectorImpl<vec3<F32> > result;
        if(pathSlot < 0 || pathSlot >= MAX_PATHSLOT || _pathStore[pathSlot].MaxVertex <= 0)
            return result;

        PATHDATA *path = &(_pathStore[pathSlot]);
        result.reserve(path->MaxVertex);
        for(I32 i = 0; i < path->MaxVertex; i++) {
            result.push_back(vec3<F32>(path->PosX[i], path->PosY[i], path->PosZ[i]));
        }

        return result;
    }

    I32 DivideRecast::getTarget(I32 pathSlot) {
        if(pathSlot < 0 || pathSlot >= MAX_PATHSLOT)
            return 0;
        
        return _pathStore[pathSlot].Target;
    }

    /// Random number generator implementation used by getRandomNavMeshPoint method.
    static F32 frand() {
        return (F32)rand()/(F32)RAND_MAX;
    }

    vec3<F32> DivideRecast::getRandomNavMeshPoint(const NavigationMesh& navMesh){
        if(navMesh.getNavQuery().getAttachedNavMesh() == nullptr)
            return VECTOR3_ZERO;

        F32 resultPoint[3];
        dtPolyRef resultPoly;
        navMesh.getNavQuery().findRandomPoint(_filter, frand, &resultPoly, resultPoint);

        return vec3<F32>(resultPoint[0], resultPoint[1], resultPoint[2]);
    }

    bool DivideRecast::findNearestPointOnNavmesh(const NavigationMesh& navMesh, const vec3<F32>& position, vec3<F32>& resultPt){
        dtPolyRef navmeshPoly;
        return findNearestPolyOnNavmesh(navMesh, position, resultPt, navmeshPoly);
    }

    bool DivideRecast::findNearestPolyOnNavmesh(const NavigationMesh& navMesh, const vec3<F32>& position, vec3<F32>& resultPt, dtPolyRef &resultPoly){
        if(navMesh.getNavQuery().getAttachedNavMesh() == nullptr){
            resultPt.set(VECTOR3_ZERO);
            return false;
        }

        F32 pt[] = {position.x, position.y, position.z};
        F32 rPt[3];
        const F32* extents = &navMesh.getExtents()[0];
        dtStatus status = navMesh.getNavQuery().findNearestPoly(pt, extents, _filter, &resultPoly, rPt);
        if((status&DT_FAILURE) || (status&DT_STATUS_DETAIL_MASK))
            return false; // couldn't find a polygon
        resultPt.set(rPt[0],rPt[1],rPt[2]);
        return true;
    }
};
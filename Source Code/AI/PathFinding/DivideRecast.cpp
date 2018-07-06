#include "Headers/DivideRecast.h"
#include "Waypoints/Headers/WaypointGraph.h"

namespace Divide {
namespace AI {
namespace Navigation {
static const U8 DT_TILECACHE_NULL_AREA = 0;
static const U8 DT_TILECACHE_WALKABLE_AREA = 63;
static const U16 DT_TILECACHE_NULL_IDX = 0xffff;

DivideRecast::DivideRecast()
{
    // Setup the default query filter
    _filter = MemoryManager_NEW dtQueryFilter();
    _filter->setIncludeFlags(0xFFFF);  // Include all
    _filter->setExcludeFlags(0);       // Exclude none
    // Area flags for polys to consider in search, and their cost
    // TODO have a way of configuring the filter
    _filter->setAreaCost(to_int(SamplePolyAreas::SAMPLE_POLYAREA_GROUND), 1.0f);
    _filter->setAreaCost(DT_TILECACHE_WALKABLE_AREA, 1.0f);

    // Init path store. MaxVertex 0 means empty path slot
    for (I32 i = 0; i < MAX_PATHSLOT; i++) {
        _pathStore[i].MaxVertex = 0;
        _pathStore[i].Target = 0;
    }
}

DivideRecast::~DivideRecast()
{
    MemoryManager::DELETE(_filter);
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
    I32 nVertCount = 0;

    // find the start polygon
    status = navQuery.findNearestPoly(pStartPos,
                                      extents,
                                      _filter,
                                      &StartPoly,
                                      StartNearest);

    if ((status & DT_FAILURE) || (status & DT_STATUS_MASK)) {
        // couldn't find a polygon
        return PathErrorCode::PATH_ERROR_NO_NEAREST_POLY_START;
    }

    // find the end polygon
    status = navQuery.findNearestPoly(pEndPos,
                                      extents,
                                      _filter,
                                      &EndPoly,
                                      EndNearest);

    if ((status & DT_FAILURE) || (status & DT_STATUS_MASK)) {
        // couldn't find a polygon
        return PathErrorCode::PATH_ERROR_NO_NEAREST_POLY_END;
    }

    status = navQuery.findPath(StartPoly,
                               EndPoly,
                               StartNearest,
                               EndNearest,
                               _filter,
                               PolyPath,
                               &nPathCount,
                               MAX_PATHPOLY);

    if ((status & DT_FAILURE) || (status & DT_STATUS_MASK)) {
        // couldn't create a path
        return PathErrorCode::PATH_ERROR_COULD_NOT_CREATE_PATH;
    }

    if (nPathCount == 0) {
        // couldn't find a path
        return PathErrorCode::PATH_ERROR_COULD_NOT_FIND_PATH;
    }

    status = navQuery.findStraightPath(StartNearest,
                                       EndNearest,
                                       PolyPath,
                                       nPathCount,
                                       StraightPath,
                                       nullptr,
                                       nullptr,
                                       &nVertCount,
                                       MAX_PATHVERT);

    if ((status & DT_FAILURE) || (status & DT_STATUS_MASK)) {
        // couldn't create a path
        return PathErrorCode::PATH_ERROR_NO_STRAIGHT_PATH_CREATE;
    }

    if (nVertCount == 0) {
        // couldn't find a path
        return PathErrorCode::PATH_ERROR_NO_STRAIGHT_PATH_FIND;
    }

    // At this point we have our path. Copy it to the path store
    for (I32 nVert = 0, nIndex = 0; nVert < nVertCount; ++nVert) {
        _pathStore[pathSlot].PosX[nVert] = StraightPath[nIndex++];
        _pathStore[pathSlot].PosY[nVert] = StraightPath[nIndex++];
        _pathStore[pathSlot].PosZ[nVert] = StraightPath[nIndex++];
    }

    _pathStore[pathSlot].MaxVertex = nVertCount;
    _pathStore[pathSlot].Target = target;

    return PathErrorCode::PATH_ERROR_NONE;
}

vectorImpl<vec3<F32> > DivideRecast::getPath(I32 pathSlot) {
    vectorImpl<vec3<F32> > result;
    if (!IS_IN_RANGE_INCLUSIVE(pathSlot, 0, MAX_PATHSLOT - 1) ||
        _pathStore[pathSlot].MaxVertex <= 0) {

        return result;
    }

    PATHDATA& path = _pathStore[pathSlot];
    result.reserve(path.MaxVertex);

    for (I32 i = 0; i < path.MaxVertex; ++i) {
        vectorAlg::emplace_back(result, path.PosX[i],
                                        path.PosY[i],
                                        path.PosZ[i]);
    }

    return result;
}

I32 DivideRecast::getTarget(I32 pathSlot) {
    if (pathSlot < 0 || pathSlot >= MAX_PATHSLOT) {
        return 0;
    }

    return _pathStore[pathSlot].Target;
}

namespace {
    /// Random number generator implementation used by navmesh query methods.
    inline F32 frand() {
        return rand() / to_float(RAND_MAX);
    }
}

bool DivideRecast::getRandomNavMeshPoint(const NavigationMesh& navMesh,
                                         vec3<F32>& resultPt) {
    if (navMesh.getNavQuery().getAttachedNavMesh() != nullptr) {
        dtPolyRef resultPoly;
        dtStatus status = navMesh.getNavQuery().findRandomPoint(_filter,
                                                                frand,
                                                                &resultPoly,
                                                                resultPt._v);

        return !(status & DT_FAILURE || status & DT_STATUS_MASK);
    }

    return false;
}

bool DivideRecast::getRandomPointAroundCircle(const NavigationMesh& navMesh,
                                              const vec3<F32>& centerPosition,
                                              F32 radius,
                                              const vec3<F32>& extents,
                                              vec3<F32>& resultPt,
                                              U8 maxIters) {

    const dtNavMeshQuery& query = navMesh.getNavQuery();

    if (query.getAttachedNavMesh() != nullptr) {
        F32 radiusSq = radius * radius;

        dtPolyRef resultPoly;
        bool pointOnPolyMesh = findNearestPolyOnNavmesh(navMesh,
                                                        centerPosition,
                                                        extents,
                                                        resultPt,
                                                        resultPoly);
        if (pointOnPolyMesh) {
            dtStatus status;
            for (U8 i = 0; i < maxIters; ++i) {
                status = query.findRandomPointAroundCircle(resultPoly,
                                                           centerPosition._v,
                                                           radius,
                                                           _filter,
                                                           frand,
                                                           &resultPoly,
                                                           resultPt._v);
                ACKNOWLEDGE_UNUSED(status);
                if (centerPosition.distanceSquared(resultPt) <= radiusSq) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool DivideRecast::findNearestPointOnNavmesh(const NavigationMesh& navMesh,
                                             const vec3<F32>& position,
                                             const vec3<F32>& extents,
                                             F32 delta,
                                             vec3<F32>& resultPt,
                                             dtPolyRef& resultPoly) {

    F32 distanceSq = delta * delta;

    bool pointOnPolyMesh = findNearestPolyOnNavmesh(navMesh,
                                                    position,
                                                    extents,
                                                    resultPt,
                                                    resultPoly);

    if (pointOnPolyMesh) {
        F32 distSQ = vec3<F32>(position.x, 0.0f, position.z).distanceSquared(vec3<F32>(resultPt.x, 0.0f, resultPt.z));
        bool pointIsInRange = (distSQ <= distanceSq && abs(position.y - resultPt.y) < extents.y); 

        if (!pointIsInRange) {
            pointOnPolyMesh = findNearestPolyOnNavmesh(navMesh,
                                                       position,
                                                       &navMesh.getExtents()[0],
                                                       resultPt,
                                                       resultPoly);
            if (pointOnPolyMesh) {
                F32 distance = position.distanceSquared(resultPt);
                pointIsInRange = distance <= distanceSq;
            }
        }
        return pointIsInRange;
    }

    return false;
}

bool DivideRecast::findNearestPolyOnNavmesh(const NavigationMesh& navMesh,
                                            const vec3<F32>& position,
                                            const vec3<F32>& extents,
                                            vec3<F32>& resultPt,
                                            dtPolyRef& resultPoly) {
    resultPt.reset();

    if (navMesh.getNavQuery().getAttachedNavMesh() != nullptr) {
        dtStatus status = navMesh.getNavQuery().findNearestPoly(position._v,
                                                                extents._v,
                                                                _filter,
                                                                &resultPoly,
                                                                resultPt._v);

        return !(status & DT_FAILURE || status & DT_STATUS_MASK);
    }

    return false;
}

};  // namespace Navigation
};  // namespace AI
};  // namespace Divide

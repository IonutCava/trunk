#include "stdafx.h"

#include "Headers/Frustum.h"
#include "Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

Frustum::Frustum()
{
    _frustumPlanes.fill(Plane<F32>(0.0f));
    _frustumPoints.fill(vec3<F32>(0.0f));
}

Frustum::FrustCollision Frustum::PlanePointIntersect(const Plane<F32>& frustumPlane, const vec3<F32>& point) const noexcept {
    switch (frustumPlane.classifyPoint(point)) {
        case Plane<F32>::Side::NO_SIDE: 
            return FrustCollision::FRUSTUM_INTERSECT;

        case Plane<F32>::Side::NEGATIVE_SIDE:
            return FrustCollision::FRUSTUM_OUT;
    }

    return FrustCollision::FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsPoint(const vec3<F32>& point, I8& lastPlaneCache) const noexcept {
    Frustum::FrustCollision res = FrustCollision::FRUSTUM_IN;
    I8 planeToSkip = -1;
    if (lastPlaneCache != -1) {
        res = PlanePointIntersect(_frustumPlanes[lastPlaneCache], point);
        if (res == FrustCollision::FRUSTUM_IN) {
            // reset cache if it's no longer valid
            planeToSkip = lastPlaneCache;
            lastPlaneCache = -1;
        }
    }

    // Cache miss: check all planes
    if (lastPlaneCache == -1) {
        for (I8 i = 0; i < to_I8(FrustPlane::COUNT); ++i) {
            if (i != planeToSkip) {
                res = PlanePointIntersect(_frustumPlanes[i], point);
                if (res != FrustCollision::FRUSTUM_IN) {
                    lastPlaneCache = i;
                    break;
                }
            }
        }
    }

    return res;
}

Frustum::FrustCollision Frustum::PlaneSphereIntersect(const Plane<F32>& frustumPlane, const vec3<F32>& center, F32 radius) const noexcept {
    F32 distance = frustumPlane.signedDistanceToPoint(center);
    if (distance < -radius) {
        return FrustCollision::FRUSTUM_OUT;
    }

    if (std::abs(distance) < radius) {
        return FrustCollision::FRUSTUM_INTERSECT;
    }

    return  FrustCollision::FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsSphere(const vec3<F32>& center, F32 radius, I8& lastPlaneCache) const noexcept {
    OPTICK_EVENT();

    Frustum::FrustCollision res = FrustCollision::FRUSTUM_IN;
    I8 planeToSkip = -1;
    if (lastPlaneCache != -1) {
        res = PlaneSphereIntersect(_frustumPlanes[lastPlaneCache], center, radius);
        if (res == FrustCollision::FRUSTUM_IN) {
            // reset cache if it's no longer valid
            planeToSkip = lastPlaneCache;
            lastPlaneCache = -1;
        }
    }

    // Cache miss: check all planes
    if (lastPlaneCache == -1) {
        for (I8 i = 0; i < to_I8(FrustPlane::COUNT); ++i) {
            if (i != planeToSkip) {
                res = PlaneSphereIntersect(_frustumPlanes[i], center, radius);
                if (res != FrustCollision::FRUSTUM_IN) {
                    lastPlaneCache = i;
                    break;
                }
            }
        }
    }

    return res;
}

Frustum::FrustCollision Frustum::PlaneBoundingBoxIntersect(const Plane<F32>& frustumPlane, const BoundingBox& bbox) const noexcept {
    const vec3<F32>& normal = frustumPlane._normal;
    if (frustumPlane.signedDistanceToPoint(bbox.getPVertex(normal)) < 0) {
        return FrustCollision::FRUSTUM_OUT;
    }
    if (frustumPlane.signedDistanceToPoint(bbox.getNVertex(normal)) < 0) {
        return FrustCollision::FRUSTUM_INTERSECT;
    }

    return FrustCollision::FRUSTUM_IN;
}


Frustum::FrustCollision Frustum::ContainsBoundingBox(const BoundingBox& bbox, I8& lastPlaneCache) const noexcept {
    OPTICK_EVENT();

    Frustum::FrustCollision res = FrustCollision::FRUSTUM_IN;
    I8 planeToSkip = -1;
    if (lastPlaneCache != -1) {
        res = PlaneBoundingBoxIntersect(_frustumPlanes[lastPlaneCache], bbox);
        if (res == FrustCollision::FRUSTUM_IN) {
            // reset cache if it's no longer valid
            planeToSkip = lastPlaneCache;
            lastPlaneCache = -1;
        }
    }

    // Cache miss: check all planes
    if (lastPlaneCache == -1) {
        for (I8 i = 0; i < to_I8(FrustPlane::COUNT); ++i) {
            if (i != planeToSkip) {
                res = PlaneBoundingBoxIntersect(_frustumPlanes[i], bbox);
                if (res != FrustCollision::FRUSTUM_IN) {
                    lastPlaneCache = i;
                    break;
                }
            }
        }
    }

    return res;
}

void Frustum::set(const Frustum& other) {
    _frustumPlanes = other._frustumPlanes;
    _frustumPoints = other._frustumPoints;
}

void Frustum::Extract(const mat4<F32>& viewMatrix, const mat4<F32>& projectionMatrix) {
    computePlanes(viewMatrix * projectionMatrix);
}

void Frustum::intersectionPoint(const Plane<F32>& a, const Plane<F32>& b, const Plane<F32>& c, vec3<F32>& outResult) noexcept {
    outResult.set((a._distance * (Cross(b._normal, c._normal))) +
                  (b._distance * (Cross(c._normal, a._normal))) +
                  (c._distance * (Cross(a._normal, b._normal))) /
                  -Dot(a._normal, Cross(b._normal, c._normal)));
}

void Frustum::updatePoints() {
    const Plane<F32>& leftPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_LEFT)];
    const Plane<F32>& rightPlane  = _frustumPlanes[to_base(FrustPlane::PLANE_RIGHT)];
    const Plane<F32>& nearPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_NEAR)];
    const Plane<F32>& farPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_FAR)];
    const Plane<F32>& topPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_TOP)];
    const Plane<F32>& bottomPlane = _frustumPlanes[to_base(FrustPlane::PLANE_BOTTOM)];

    intersectionPoint(nearPlane, leftPlane,  topPlane,    _frustumPoints[to_base(FrustPoints::NEAR_LEFT_TOP)]);
    intersectionPoint(nearPlane, rightPlane, topPlane,    _frustumPoints[to_base(FrustPoints::NEAR_RIGHT_TOP)]);
    intersectionPoint(nearPlane, rightPlane, bottomPlane, _frustumPoints[to_base(FrustPoints::NEAR_RIGHT_BOTTOM)]);
    intersectionPoint(nearPlane, leftPlane,  bottomPlane, _frustumPoints[to_base(FrustPoints::NEAR_LEFT_BOTTOM)]);
    intersectionPoint(farPlane,  leftPlane,  topPlane,    _frustumPoints[to_base(FrustPoints::FAR_LEFT_TOP)]);
    intersectionPoint(farPlane,  rightPlane, topPlane,    _frustumPoints[to_base(FrustPoints::FAR_RIGHT_TOP)]);
    intersectionPoint(farPlane,  rightPlane, bottomPlane, _frustumPoints[to_base(FrustPoints::FAR_RIGHT_BOTTOM)]);
    intersectionPoint(farPlane,  leftPlane,  bottomPlane, _frustumPoints[to_base(FrustPoints::FAR_LEFT_BOTTOM)]);
}

// Get the frustum corners in WorldSpace.
void Frustum::getCornersWorldSpace(std::array<vec3<F32>, 8 >& cornersWS) const {
    cornersWS = _frustumPoints;
}

// Get the frustum corners in ViewSpace.
void Frustum::getCornersViewSpace(const mat4<F32>& viewMatrix, std::array<vec3<F32>, 8 >& cornersVS) const {
    getCornersWorldSpace(cornersVS);

    std::transform(std::begin(cornersVS), std::end(cornersVS), std::begin(cornersVS),
                   [&viewMatrix](vec3<F32>& pt) {
                       return viewMatrix.transformHomogeneous(pt);
                   });
}

void Frustum::computePlanes(const mat4<F32>& viewProjMatrix) {
#if 0
    computeFrustumPlanes(viewProjMatrix, _frustumPlanes);
#else
    Plane<F32>& leftPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_LEFT)];
    Plane<F32>& rightPlane  = _frustumPlanes[to_base(FrustPlane::PLANE_RIGHT)];
    Plane<F32>& nearPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_NEAR)];
    Plane<F32>& farPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_FAR)];
    Plane<F32>& topPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_TOP)];
    Plane<F32>& bottomPlane = _frustumPlanes[to_base(FrustPlane::PLANE_BOTTOM)];

    F32 const* mat = &viewProjMatrix.mat[0];

    leftPlane.set(   mat[3] + mat[0], mat[7] + mat[4], mat[11] + mat[8],  mat[15] + mat[12]);
    rightPlane.set(  mat[3] - mat[0], mat[7] - mat[4], mat[11] - mat[8],  mat[15] - mat[12]);
    topPlane.set(    mat[3] - mat[1], mat[7] - mat[5], mat[11] - mat[9],  mat[15] - mat[13]);
    bottomPlane.set( mat[3] + mat[1], mat[7] + mat[5], mat[11] + mat[9],  mat[15] + mat[13]);
    nearPlane.set(   mat[3] + mat[2], mat[7] + mat[6], mat[11] + mat[10], mat[15] + mat[14]);
    farPlane.set(    mat[3] - mat[2], mat[7] - mat[6], mat[11] - mat[10], mat[15] - mat[14]);

    for (Plane<F32>& plane : _frustumPlanes) {
        plane.normalize();
    }
#endif

    updatePoints();
}

void Frustum::computePlanes(const mat4<F32>& invViewProj, Plane<F32>* planesOut) {
    std::array<vec4<F32>, to_base(Frustum::FrustPlane::COUNT)> planesTemp;

    computePlanes(invViewProj, planesTemp.data());
    for (U8 i = 0; i < to_U8(Frustum::FrustPoints::COUNT); ++i) {
        planesOut[i].set(planesTemp[i]);
    }
}

void Frustum::computePlanes(const mat4<F32>& invViewProj, vec4<F32>* planesOut) {
    // no constexpr due to alignment declspec
    static const vec4<I32> unitVecs[] = { vec4<I32>(-1, -1, -1, 1),
                                          vec4<I32>(-1 , 1, -1, 1),
                                          vec4<I32>(-1, -1,  1, 1),
                                          vec4<I32>( 1, -1, -1, 1),
                                          vec4<I32>( 1,  1, -1, 1),
                                          vec4<I32>( 1, -1,  1, 1),
                                          vec4<I32>( 1,  1,  1, 1) };

    // Get world-space coordinates for clip-space bounds.
    const vec4<F32> lbn(invViewProj * unitVecs[0]);
    const vec4<F32> ltn(invViewProj * unitVecs[1]);
    const vec4<F32> lbf(invViewProj * unitVecs[2]);
    const vec4<F32> rbn(invViewProj * unitVecs[3]);
    const vec4<F32> rtn(invViewProj * unitVecs[4]);
    const vec4<F32> rbf(invViewProj * unitVecs[5]);
    const vec4<F32> rtf(invViewProj * unitVecs[6]);

    const vec3<F32> lbn_pos(lbn / lbn.w);
    const vec3<F32> ltn_pos(ltn / ltn.w);
    const vec3<F32> lbf_pos(lbf / lbf.w);
    const vec3<F32> rbn_pos(rbn / rbn.w);
    const vec3<F32> rtn_pos(rtn / rtn.w);
    const vec3<F32> rbf_pos(rbf / rbf.w);
    const vec3<F32> rtf_pos(rtf / rtf.w);

    // Get plane equations for all sides of frustum.
    const vec3<F32> left_normal(  Normalized(Cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos)));
    const vec3<F32> right_normal( Normalized(Cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos)));
    const vec3<F32> top_normal(   Normalized(Cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos)));
    const vec3<F32> bottom_normal(Normalized(Cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos)));
    const vec3<F32> near_normal(  Normalized(Cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos)));
    const vec3<F32> far_normal(   Normalized(Cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos)));

    planesOut[to_base(Frustum::FrustPlane::PLANE_LEFT)].set(  left_normal,   -Dot(left_normal, lbn_pos));
    planesOut[to_base(Frustum::FrustPlane::PLANE_RIGHT)].set( right_normal,  -Dot(right_normal, rbn_pos));
    planesOut[to_base(Frustum::FrustPlane::PLANE_NEAR)].set(  near_normal,   -Dot(near_normal, lbn_pos));
    planesOut[to_base(Frustum::FrustPlane::PLANE_FAR)].set(   far_normal,    -Dot(far_normal, lbf_pos));
    planesOut[to_base(Frustum::FrustPlane::PLANE_TOP)].set(   top_normal,    -Dot(top_normal, ltn_pos));
    planesOut[to_base(Frustum::FrustPlane::PLANE_BOTTOM)].set(bottom_normal, -Dot(bottom_normal, lbn_pos));
}
};
#include "stdafx.h"

#include "Headers/Frustum.h"
#include "Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

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
    const F32 distance = frustumPlane.signedDistanceToPoint(center);
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


void Frustum::updatePoints() noexcept {
    const Plane<F32>& leftPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_LEFT)];
    const Plane<F32>& rightPlane  = _frustumPlanes[to_base(FrustPlane::PLANE_RIGHT)];
    const Plane<F32>& nearPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_NEAR)];
    const Plane<F32>& farPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_FAR)];
    const Plane<F32>& topPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_TOP)];
    const Plane<F32>& bottomPlane = _frustumPlanes[to_base(FrustPlane::PLANE_BOTTOM)];

    const auto intersectionPoint = [](const Plane<F32> & a, const Plane<F32> & b, const Plane<F32> & c) noexcept {
        const F32 denom = Dot(Cross(a._normal, b._normal), c._normal);
        assert(!IS_ZERO(denom));
        return (-(a._distance * Cross(b._normal, c._normal)) -
                 (b._distance * Cross(c._normal, a._normal)) -
                 (c._distance * Cross(a._normal, b._normal))) / denom;
    };

    _frustumPoints[to_base(FrustPoints::NEAR_LEFT_TOP)]     = intersectionPoint(nearPlane, leftPlane,  topPlane);
    _frustumPoints[to_base(FrustPoints::NEAR_RIGHT_TOP)]    = intersectionPoint(nearPlane, rightPlane, topPlane);
    _frustumPoints[to_base(FrustPoints::NEAR_LEFT_BOTTOM)]  = intersectionPoint(nearPlane, leftPlane,  bottomPlane);
    _frustumPoints[to_base(FrustPoints::NEAR_RIGHT_BOTTOM)] = intersectionPoint(nearPlane, rightPlane, bottomPlane);
    _frustumPoints[to_base(FrustPoints::FAR_LEFT_TOP)]      = intersectionPoint(farPlane,  leftPlane,  topPlane);
    _frustumPoints[to_base(FrustPoints::FAR_RIGHT_TOP)]     = intersectionPoint(farPlane,  rightPlane, topPlane);
    _frustumPoints[to_base(FrustPoints::FAR_LEFT_BOTTOM)]   = intersectionPoint(farPlane,  leftPlane,  bottomPlane);
    _frustumPoints[to_base(FrustPoints::FAR_RIGHT_BOTTOM)]  = intersectionPoint(farPlane,  rightPlane, bottomPlane);
}

// Get the frustum corners in WorldSpace.
void Frustum::getCornersWorldSpace(std::array<vec3<F32>, 8 >& cornersWS) const noexcept {
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
    F32* leftPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_LEFT)]._equation._v;
    F32* rightPlane  = _frustumPlanes[to_base(FrustPlane::PLANE_RIGHT)]._equation._v;
    F32* nearPlane   = _frustumPlanes[to_base(FrustPlane::PLANE_NEAR)]._equation._v;
    F32* farPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_FAR)]._equation._v;
    F32* topPlane    = _frustumPlanes[to_base(FrustPlane::PLANE_TOP)]._equation._v;
    F32* bottomPlane = _frustumPlanes[to_base(FrustPlane::PLANE_BOTTOM)]._equation._v;

    const auto& mat = viewProjMatrix.m;

    for (I8 i = 4; i--; ) { leftPlane[i] = mat[i][3] + mat[i][0]; }
    for (I8 i = 4; i--; ) { rightPlane[i] = mat[i][3] - mat[i][0]; }
    for (I8 i = 4; i--; ) { bottomPlane[i] = mat[i][3] + mat[i][1]; }
    for (I8 i = 4; i--; ) { topPlane[i] = mat[i][3] - mat[i][1]; }
    for (I8 i = 4; i--; ) { nearPlane[i] = mat[i][3] + mat[i][2]; }
    for (I8 i = 4; i--; ) { farPlane[i] = mat[i][3] - mat[i][2]; }

    for (Plane<F32>& plane : _frustumPlanes) {
        plane.normalize();
    }

    updatePoints();
}

};
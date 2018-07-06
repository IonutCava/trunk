#include "Headers/Frustum.h"
#include "Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

Frustum::Frustum(Camera& parentCamera)
    : _parentCamera(parentCamera),
      _pointsDirty(true)
{
}

Frustum::FrustCollision Frustum::ContainsPoint(const vec3<F32>& point) const {
    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        if (frustumPlane.classifyPoint(point) != Plane<F32>::Side::POSITIVE_SIDE) {
            return FrustCollision::FRUSTUM_OUT;
        }
    }

    return FrustCollision::FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsSphere(const vec3<F32>& center,
                                                F32 radius) const {
    F32 distance = 0.0f;

    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        distance = frustumPlane.signedDistanceToPoint(center);

        if (distance < -radius) {
            return FrustCollision::FRUSTUM_OUT;
        }

        if (std::fabs(distance) < radius) {
            return FrustCollision::FRUSTUM_INTERSECT;
        }
    }

    return FrustCollision::FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsBoundingBox(const BoundingBox& bbox) const {
    //const vec3<F32>* box[] = {&bbox.getMin(), &bbox.getMax()};
    vec3<F32> vmin, vmax;
    vec3<F32> bmin = bbox.getMin();
    vec3<F32> bmax = bbox.getMax();
    FrustCollision ret = FrustCollision::FRUSTUM_IN;
    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        // p-vertex selection (with the index trick)
        // According to the plane normal we can know the
        // indices of the positive vertex
        const vec3<F32>& normal = frustumPlane.getNormal();
        if (frustumPlane.signedDistanceToPoint(bbox.getPVertex(normal)) < 0) {
            return FrustCollision::FRUSTUM_OUT;
        }

        if (frustumPlane.signedDistanceToPoint(bbox.getNVertex(normal)) < 0) {
            ret = FrustCollision::FRUSTUM_INTERSECT;
        }
    }

    return ret;
}

void Frustum::Extract(const mat4<F32>& viewMatrix, const mat4<F32>& projectionMatrix) {

    mat4<F32>::Multiply(viewMatrix, projectionMatrix, _viewProjectionMatrixCache);

#if 0
    GFXDevice::computeFrustumPlanes(_viewProjectionMatrixCache.getInverse(), _frustumPlanes);
#else
    Plane<F32>& leftPlane   = _frustumPlanes[to_const_uint(FrustPlane::PLANE_LEFT)];
    Plane<F32>& rightPlane  = _frustumPlanes[to_const_uint(FrustPlane::PLANE_RIGHT)];
    Plane<F32>& nearPlane   = _frustumPlanes[to_const_uint(FrustPlane::PLANE_NEAR)];
    Plane<F32>& farPlane    = _frustumPlanes[to_const_uint(FrustPlane::PLANE_FAR)];
    Plane<F32>& topPlane    = _frustumPlanes[to_const_uint(FrustPlane::PLANE_TOP)];
    Plane<F32>& bottomPlane = _frustumPlanes[to_const_uint(FrustPlane::PLANE_BOTTOM)];

    const F32* const mat = &_viewProjectionMatrixCache.mat[0];

    leftPlane.set(  mat[3] + mat[0], mat[7] + mat[4], mat[11] + mat[8],  mat[15] + mat[12]);
    rightPlane.set( mat[3] - mat[0], mat[7] - mat[4], mat[11] - mat[8],  mat[15] - mat[12]);
    topPlane.set(   mat[3] - mat[1], mat[7] - mat[5], mat[11] - mat[9],  mat[15] - mat[13]);
    bottomPlane.set(mat[3] + mat[1], mat[7] + mat[5], mat[11] + mat[9],  mat[15] + mat[13]);
    nearPlane.set(  mat[3] + mat[2], mat[7] + mat[6], mat[11] + mat[10], mat[15] + mat[14]);
    farPlane.set(   mat[3] - mat[2], mat[7] - mat[6], mat[11] - mat[10], mat[15] - mat[14]);

    for (Plane<F32>& plane : _frustumPlanes) {
        plane.normalize();
    }
#endif

    _pointsDirty = true;
}

void Frustum::intersectionPoint(const Plane<F32>& a, const Plane<F32>& b,
                                const Plane<F32>& c, vec3<F32>& outResult) {
    outResult.set((a.getDistance() * (Cross(b.getNormal(), c.getNormal()))) +
                  (b.getDistance() * (Cross(c.getNormal(), a.getNormal()))) +
                  (c.getDistance() * (Cross(a.getNormal(), b.getNormal()))) /
                  -Dot(a.getNormal(), Cross(b.getNormal(), c.getNormal())));
}

void Frustum::updatePoints() {
    if (_pointsDirty) {
        const Plane<F32>& leftPlane   = _frustumPlanes[to_const_uint(FrustPlane::PLANE_LEFT)];
        const Plane<F32>& rightPlane  = _frustumPlanes[to_const_uint(FrustPlane::PLANE_RIGHT)];
        const Plane<F32>& nearPlane   = _frustumPlanes[to_const_uint(FrustPlane::PLANE_NEAR)];
        const Plane<F32>& farPlane    = _frustumPlanes[to_const_uint(FrustPlane::PLANE_FAR)];
        const Plane<F32>& topPlane    = _frustumPlanes[to_const_uint(FrustPlane::PLANE_TOP)];
        const Plane<F32>& bottomPlane = _frustumPlanes[to_const_uint(FrustPlane::PLANE_BOTTOM)];

        intersectionPoint(nearPlane, leftPlane,  topPlane,    _frustumPoints[to_const_uint(FrustPoints::NEAR_LEFT_TOP)]);
        intersectionPoint(nearPlane, rightPlane, topPlane,    _frustumPoints[to_const_uint(FrustPoints::NEAR_RIGHT_TOP)]);
        intersectionPoint(nearPlane, rightPlane, bottomPlane, _frustumPoints[to_const_uint(FrustPoints::NEAR_RIGHT_BOTTOM)]);
        intersectionPoint(nearPlane, leftPlane,  bottomPlane, _frustumPoints[to_const_uint(FrustPoints::NEAR_LEFT_BOTTOM)]);
        intersectionPoint(farPlane,  leftPlane,  topPlane,    _frustumPoints[to_const_uint(FrustPoints::FAR_LEFT_TOP)]);
        intersectionPoint(farPlane,  rightPlane, topPlane,    _frustumPoints[to_const_uint(FrustPoints::FAR_RIGHT_TOP)]);
        intersectionPoint(farPlane,  rightPlane, bottomPlane, _frustumPoints[to_const_uint(FrustPoints::FAR_RIGHT_BOTTOM)]);
        intersectionPoint(farPlane,  leftPlane,  bottomPlane, _frustumPoints[to_const_uint(FrustPoints::FAR_LEFT_BOTTOM)]);

        _pointsDirty = false;
    }
}

// Get the frustum corners in WorldSpace.
void Frustum::getCornersWorldSpace(vectorImpl<vec3<F32> >& cornersWS) {
    updatePoints();

    cornersWS.resize(0);
    cornersWS.insert(std::begin(cornersWS), std::cbegin(_frustumPoints), std::cend(_frustumPoints));
}

// Get the frustum corners in ViewSpace.
void Frustum::getCornersViewSpace(vectorImpl<vec3<F32> >& cornersVS) {
    getCornersWorldSpace(cornersVS);

    const mat4<F32>& viewMatrix = _parentCamera.getViewMatrix();
    std::transform(std::begin(cornersVS), std::end(cornersVS), std::begin(cornersVS),
                   [&viewMatrix](vec3<F32>& pt) {
                       return viewMatrix.transformHomogeneous(pt);
                   });
}
};
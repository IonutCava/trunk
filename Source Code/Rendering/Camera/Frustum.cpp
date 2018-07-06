#include "Headers/Frustum.h"
#include "Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

Frustum::Frustum(Camera& parentCamera)
    : _pointsDirty(true), _parentCamera(parentCamera) {}

Frustum::FrustCollision Frustum::ContainsPoint(const vec3<F32>& point) const {
    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        if (frustumPlane.classifyPoint(point) !=
            Plane<F32>::Side::POSITIVE_SIDE) {
            return FrustCollision::FRUSTUM_OUT;
        }
    }

    return FrustCollision::FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsSphere(const vec3<F32>& center,
                                                F32 radius) const {
    F32 distance = 0.0f;

    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        distance = frustumPlane.getDistance(center);

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
    const vec3<F32>* box[] = {&bbox.getMin(), &bbox.getMax()};
    vec3<F32> vmin, vmax;
    vec3<F32> bmin = bbox.getMin();
    vec3<F32> bmax = bbox.getMax();
    FrustCollision ret = FrustCollision::FRUSTUM_IN;
    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        // p-vertex selection (with the index trick)
        // According to the plane normal we can know the
        // indices of the positive vertex
        const vec3<F32>& normal = frustumPlane.getNormal();
        /*const I32 px = static_cast<I32>(p.x < 0.0f);
        const I32 py = static_cast<I32>(p.y < 0.0f);
        const I32 pz = static_cast<I32>(p.z < 0.0f);

        vmin.set(box[1 - px]->x, box[1 - py]->y, box[1 - pz]->z);
        vmax.set(box[px]->x, box[py]->y, box[pz]->z);*/


        if (frustumPlane.getDistance(bbox.getPVertex(normal)) < 0) {
            return FrustCollision::FRUSTUM_OUT;
        }

        if (frustumPlane.getDistance(bbox.getNVertex(normal)) < 0) {
            ret = FrustCollision::FRUSTUM_INTERSECT;
        }
    }

    return ret;
}

void Frustum::Extract() {
    GFX_DEVICE.getMatrix(MATRIX::VIEW_PROJECTION, _viewProjectionMatrixCache);

    Plane<F32>& rightPlane = _frustumPlanes[0];
    Plane<F32>& leftPlane = _frustumPlanes[1];
    Plane<F32>& bottomPlane = _frustumPlanes[2];
    Plane<F32>& topPlane = _frustumPlanes[3];
    Plane<F32>& farPlane = _frustumPlanes[4];
    Plane<F32>& nearPlane = _frustumPlanes[5];

    const F32* mat = &_viewProjectionMatrixCache.mat[0];

    rightPlane.set(mat[3] - mat[0], mat[7] - mat[4], mat[11] - mat[8], mat[15] - mat[12]);
    rightPlane.normalize();

    leftPlane.set(mat[3] + mat[0], mat[7] + mat[4], mat[11] + mat[8], mat[15] + mat[12]);
    leftPlane.normalize();

    bottomPlane.set(mat[3] + mat[1], mat[7] + mat[5], mat[11] + mat[9], mat[15] + mat[13]);
    bottomPlane.normalize();

    topPlane.set(mat[3] - mat[1], mat[7] - mat[5], mat[11] - mat[9], mat[15] - mat[13]);
    topPlane.normalize();

    farPlane.set(mat[3] - mat[2], mat[7] - mat[6], mat[11] - mat[10], mat[15] - mat[14]);
    farPlane.normalize();

    nearPlane.set(mat[3] + mat[2], mat[7] + mat[6], mat[11] + mat[10], mat[15] + mat[14]);
    nearPlane.normalize();

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
    if (!_pointsDirty) {
        return;
    }

    const Plane<F32>& rightPlane = _frustumPlanes[0];
    const Plane<F32>& leftPlane = _frustumPlanes[1];
    const Plane<F32>& bottomPlane = _frustumPlanes[2];
    const Plane<F32>& topPlane = _frustumPlanes[3];
    const Plane<F32>& farPlane = _frustumPlanes[4];
    const Plane<F32>& nearPlane = _frustumPlanes[5];

    intersectionPoint(nearPlane, leftPlane, topPlane, _frustumPoints[0]);
    intersectionPoint(nearPlane, rightPlane, topPlane, _frustumPoints[1]);
    intersectionPoint(nearPlane, rightPlane, bottomPlane, _frustumPoints[2]);
    intersectionPoint(nearPlane, leftPlane, bottomPlane, _frustumPoints[3]);
    intersectionPoint(farPlane, leftPlane, topPlane, _frustumPoints[4]);
    intersectionPoint(farPlane, rightPlane, topPlane, _frustumPoints[5]);
    intersectionPoint(farPlane, rightPlane, bottomPlane, _frustumPoints[6]);
    intersectionPoint(farPlane, leftPlane, bottomPlane, _frustumPoints[7]);

    _pointsDirty = false;
}

// Get the frustum corners in WorldSpace. cornerWS must be a vector with at
// least 8 allocated slots
void Frustum::getCornersWorldSpace(vectorImpl<vec3<F32> >& cornersWS) {
    assert(cornersWS.size() >= 8);

    updatePoints();

    for (U8 i = 0; i < 8; ++i) {
        cornersWS[i].set(_frustumPoints[i]);
    }
}

// Get the frustum corners in ViewSpace. cornerVS must be a vector with at least
// 8 allocated slots
void Frustum::getCornersViewSpace(vectorImpl<vec3<F32> >& cornersVS) {
    assert(cornersVS.size() >= 8);

    updatePoints();

    const mat4<F32>& viewMatrix = _parentCamera.getViewMatrix();
    for (U8 i = 0; i < 8; ++i) {
        cornersVS[i].set(viewMatrix.transformHomogeneous(_frustumPoints[i]));
    }
}
};
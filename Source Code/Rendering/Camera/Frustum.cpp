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

    computePlanes(_viewProjectionMatrixCache);
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
        const Plane<F32>& leftPlane   = _frustumPlanes[to_const_U32(FrustPlane::PLANE_LEFT)];
        const Plane<F32>& rightPlane  = _frustumPlanes[to_const_U32(FrustPlane::PLANE_RIGHT)];
        const Plane<F32>& nearPlane   = _frustumPlanes[to_const_U32(FrustPlane::PLANE_NEAR)];
        const Plane<F32>& farPlane    = _frustumPlanes[to_const_U32(FrustPlane::PLANE_FAR)];
        const Plane<F32>& topPlane    = _frustumPlanes[to_const_U32(FrustPlane::PLANE_TOP)];
        const Plane<F32>& bottomPlane = _frustumPlanes[to_const_U32(FrustPlane::PLANE_BOTTOM)];

        intersectionPoint(nearPlane, leftPlane,  topPlane,    _frustumPoints[to_const_U32(FrustPoints::NEAR_LEFT_TOP)]);
        intersectionPoint(nearPlane, rightPlane, topPlane,    _frustumPoints[to_const_U32(FrustPoints::NEAR_RIGHT_TOP)]);
        intersectionPoint(nearPlane, rightPlane, bottomPlane, _frustumPoints[to_const_U32(FrustPoints::NEAR_RIGHT_BOTTOM)]);
        intersectionPoint(nearPlane, leftPlane,  bottomPlane, _frustumPoints[to_const_U32(FrustPoints::NEAR_LEFT_BOTTOM)]);
        intersectionPoint(farPlane,  leftPlane,  topPlane,    _frustumPoints[to_const_U32(FrustPoints::FAR_LEFT_TOP)]);
        intersectionPoint(farPlane,  rightPlane, topPlane,    _frustumPoints[to_const_U32(FrustPoints::FAR_RIGHT_TOP)]);
        intersectionPoint(farPlane,  rightPlane, bottomPlane, _frustumPoints[to_const_U32(FrustPoints::FAR_RIGHT_BOTTOM)]);
        intersectionPoint(farPlane,  leftPlane,  bottomPlane, _frustumPoints[to_const_U32(FrustPoints::FAR_LEFT_BOTTOM)]);

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

void Frustum::computePlanes(const mat4<F32>& invViewProj) {
#if 0
    computeFrustumPlanes(invViewProj.getInverse(), _frustumPlanes);
#else
    Plane<F32>& leftPlane   = _frustumPlanes[to_const_U32(FrustPlane::PLANE_LEFT)];
    Plane<F32>& rightPlane  = _frustumPlanes[to_const_U32(FrustPlane::PLANE_RIGHT)];
    Plane<F32>& nearPlane   = _frustumPlanes[to_const_U32(FrustPlane::PLANE_NEAR)];
    Plane<F32>& farPlane    = _frustumPlanes[to_const_U32(FrustPlane::PLANE_FAR)];
    Plane<F32>& topPlane    = _frustumPlanes[to_const_U32(FrustPlane::PLANE_TOP)];
    Plane<F32>& bottomPlane = _frustumPlanes[to_const_U32(FrustPlane::PLANE_BOTTOM)];

    F32 const* mat = &invViewProj.mat[0];

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

    _pointsDirty = true;
}

void Frustum::computePlanes(const mat4<F32>& invViewProj, Plane<F32>* planesOut) {
    std::array<vec4<F32>, to_const_U32(Frustum::FrustPlane::COUNT)> planesTemp;

    computePlanes(invViewProj, planesTemp.data());
    for (U8 i = 0; i < to_U8(Frustum::FrustPoints::COUNT); ++i) {
        planesOut[i].set(planesTemp[i]);
    }
}

void Frustum::computePlanes(const mat4<F32>& invViewProj, vec4<F32>* planesOut) {
    static const vec4<F32> unitVecs[] = { vec4<F32>(-1, -1, -1, 1),
                                          vec4<F32>(-1 , 1, -1, 1),
                                          vec4<F32>(-1, -1,  1, 1),
                                          vec4<F32>(1, -1, -1, 1),
                                          vec4<F32>(1,  1, -1, 1),
                                          vec4<F32>(1, -1,  1, 1),
                                          vec4<F32>(1,  1,  1, 1) };

    // Get world-space coordinates for clip-space bounds.
    vec4<F32> lbn(invViewProj * unitVecs[0]);
    vec4<F32> ltn(invViewProj * unitVecs[1]);
    vec4<F32> lbf(invViewProj * unitVecs[2]);
    vec4<F32> rbn(invViewProj * unitVecs[3]);
    vec4<F32> rtn(invViewProj * unitVecs[4]);
    vec4<F32> rbf(invViewProj * unitVecs[5]);
    vec4<F32> rtf(invViewProj * unitVecs[6]);

    vec3<F32> lbn_pos(lbn.xyz() / lbn.w);
    vec3<F32> ltn_pos(ltn.xyz() / ltn.w);
    vec3<F32> lbf_pos(lbf.xyz() / lbf.w);
    vec3<F32> rbn_pos(rbn.xyz() / rbn.w);
    vec3<F32> rtn_pos(rtn.xyz() / rtn.w);
    vec3<F32> rbf_pos(rbf.xyz() / rbf.w);
    vec3<F32> rtf_pos(rtf.xyz() / rtf.w);

    // Get plane equations for all sides of frustum.
    vec3<F32> left_normal(Cross(lbf_pos - lbn_pos, ltn_pos - lbn_pos));
    left_normal.normalize();
    vec3<F32> right_normal(Cross(rtn_pos - rbn_pos, rbf_pos - rbn_pos));
    right_normal.normalize();
    vec3<F32> top_normal(Cross(ltn_pos - rtn_pos, rtf_pos - rtn_pos));
    top_normal.normalize();
    vec3<F32> bottom_normal(Cross(rbf_pos - rbn_pos, lbn_pos - rbn_pos));
    bottom_normal.normalize();
    vec3<F32> near_normal(Cross(ltn_pos - lbn_pos, rbn_pos - lbn_pos));
    near_normal.normalize();
    vec3<F32> far_normal(Cross(rtf_pos - rbf_pos, lbf_pos - rbf_pos));
    far_normal.normalize();

    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_LEFT)].set(left_normal, -Dot(left_normal, lbn_pos));
    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_RIGHT)].set(right_normal, -Dot(right_normal, rbn_pos));
    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_NEAR)].set(near_normal, -Dot(near_normal, lbn_pos));
    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_FAR)].set(far_normal, -Dot(far_normal, lbf_pos));
    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_TOP)].set(top_normal, -Dot(top_normal, ltn_pos));
    planesOut[to_const_U32(Frustum::FrustPlane::PLANE_BOTTOM)].set(bottom_normal, -Dot(bottom_normal, lbn_pos));
}
};
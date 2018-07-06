#include "Headers/Frustum.h"
#include "Headers/Camera.h"

#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

namespace Divide {

Frustum::Frustum(Camera& parentCamera) : _pointsDirty(true),
                                         _parentCamera(parentCamera)
{
}

Frustum::FrustCollision Frustum::ContainsPoint(const vec3<F32>& point) const {
    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        if (frustumPlane.classifyPoint(point) != Plane<F32>::POSITIVE_SIDE) {
            return FRUSTUM_OUT;
        }
    }
    
    return FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsSphere(const vec3<F32>& center, F32 radius) const {
    F32 distance = 0.0f;

    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        distance = frustumPlane.getDistance(center);

        if (distance < -radius) {     
            return FRUSTUM_OUT;
        }

        if (std::fabs(distance) < radius) {
            return FRUSTUM_INTERSECT;
        }
    }

    return FRUSTUM_IN;
}

Frustum::FrustCollision Frustum::ContainsBoundingBox(const BoundingBox& bbox) const {
    const vec3<F32> *boxCorners = bbox.getPoints();

    I32 iPtIn = 1, iTotalIn = 0, iInCount = 8;

    for (const Plane<F32>& frustumPlane : _frustumPlanes) {
        iInCount = 8;
        iPtIn = 1;

        for (U8 c = 0; c < 8; ++c) {
            if (frustumPlane.classifyPoint(boxCorners[c]) == Plane<F32>::NEGATIVE_SIDE) {
                iPtIn = 0;
                iInCount--;
            }
        }

        if (iInCount == 0) {
            return FRUSTUM_OUT;
        }

        iTotalIn += iPtIn;
    }

    return iTotalIn == 6 ? FRUSTUM_IN : FRUSTUM_INTERSECT;
}

void Frustum::Extract() {

    _viewProjectionMatrixCache.set(GFX_DEVICE.getMatrix(VIEW_PROJECTION_MATRIX));
    
	if (_viewProjectionMatrixCacheOld == _viewProjectionMatrixCache) {
        return;
	}

    _viewProjectionMatrixCacheOld.set(_viewProjectionMatrixCache);

    Plane<F32>& rightPlane  = _frustumPlanes[0];
    Plane<F32>& leftPlane   = _frustumPlanes[1];
    Plane<F32>& bottomPlane = _frustumPlanes[2];
    Plane<F32>& topPlane    = _frustumPlanes[3];
    Plane<F32>& farPlane    = _frustumPlanes[4];
    Plane<F32>& nearPlane   = _frustumPlanes[5];

    const F32* mat = &_viewProjectionMatrixCache.mat[0];

    rightPlane.set(mat[ 3] - mat[ 0], 
                   mat[ 7] - mat[ 4], 
                   mat[11] - mat[ 8], 
                   mat[15] - mat[12]); 
    rightPlane.normalize();

    leftPlane.set(mat[ 3] + mat[ 0],
                  mat[ 7] + mat[ 4],
                  mat[11] + mat[ 8], 
                  mat[15] + mat[12]);
    leftPlane.normalize();

    bottomPlane.set(mat[ 3] + mat[ 1], 
                    mat[ 7] + mat[ 5], 
                    mat[11] + mat[ 9], 
                    mat[15] + mat[13]); 
    bottomPlane.normalize();

    topPlane.set(mat[ 3] - mat[ 1], 
                 mat[ 7] - mat[ 5], 
                 mat[11] - mat[ 9], 
                 mat[15] - mat[13]);
    topPlane.normalize();

    farPlane.set(mat[ 3] - mat[ 2], 
                 mat[ 7] - mat[ 6], 
                 mat[11] - mat[10], 
                 mat[15] - mat[14]); 
    farPlane.normalize();

    nearPlane.set(mat[ 3] + mat[ 2], 
                  mat[ 7] + mat[ 6], 
                  mat[11] + mat[10], 
                  mat[15] + mat[14]); 
    nearPlane.normalize();


    _pointsDirty = true;
}

void Frustum::intersectionPoint(const Plane<F32> & a, 
                                const Plane<F32> & b, 
                                const Plane<F32> & c, 
                                vec3<F32>& outResult) {

    outResult.set((a.getDistance() * (cross(b.getNormal(), c.getNormal()))) + 
                  (b.getDistance() * (cross(c.getNormal(), a.getNormal()))) + 
                  (c.getDistance() * (cross(a.getNormal(), b.getNormal()))) / 
                  -dot(a.getNormal(), cross(b.getNormal(), c.getNormal())));
}

void Frustum::updatePoints() {
    if (!_pointsDirty) {
        return;
    }

    const Plane<F32>& rightPlane  = _frustumPlanes[0];
    const Plane<F32>& leftPlane   = _frustumPlanes[1];
    const Plane<F32>& bottomPlane = _frustumPlanes[2];
    const Plane<F32>& topPlane    = _frustumPlanes[3];
    const Plane<F32>& farPlane    = _frustumPlanes[4];
    const Plane<F32>& nearPlane   = _frustumPlanes[5];

    intersectionPoint(nearPlane, leftPlane,  topPlane,    _frustumPoints[0]);
    intersectionPoint(nearPlane, rightPlane, topPlane,    _frustumPoints[1]);
    intersectionPoint(nearPlane, rightPlane, bottomPlane, _frustumPoints[2]);
    intersectionPoint(nearPlane, leftPlane,  bottomPlane, _frustumPoints[3]);
    intersectionPoint(farPlane,  leftPlane,  topPlane,    _frustumPoints[4]);
    intersectionPoint(farPlane,  rightPlane, topPlane,    _frustumPoints[5]);
    intersectionPoint(farPlane,  rightPlane, bottomPlane, _frustumPoints[6]);
    intersectionPoint(farPlane,  leftPlane,  bottomPlane, _frustumPoints[7]);

    _pointsDirty = false;
}

// Get the frustum corners in WorldSpace. cornerWS must be a vector with at least 8 allocated slots
void Frustum::getCornersWorldSpace(vectorImpl<vec3<F32> >& cornersWS){
    assert(cornersWS.size() >= 8);

    updatePoints();

    for (U8 i = 0; i < 8; ++i) {
        cornersWS[i].set(_frustumPoints[i]);
    }

}

// Get the frustum corners in ViewSpace. cornerVS must be a vector with at least 8 allocated slots
void Frustum::getCornersViewSpace(vectorImpl<vec3<F32> >& cornersVS){
    assert(cornersVS.size() >= 8);

    updatePoints();

    const mat4<F32>& viewMatrix = _parentCamera.getViewMatrix();
    for (U8 i = 0; i < 8; ++i) {
        cornersVS[i].set(viewMatrix.transform(_frustumPoints[i]));
    }
}

};
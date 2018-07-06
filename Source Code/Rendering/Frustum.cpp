#include "Headers/Frustum.h"

#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

Frustum::Frustum() : Singleton(), _pointsDirty(true)
{
}

bool Frustum::ContainsPoint(const vec3<F32>& point) const {
    for (const Plane<F32>& frustumPlane : _frustumPlanes){
        if (frustumPlane.classifyPoint(point) != Plane<F32>::POSITIVE_SIDE)
            return false;
    }
    
    return true;
}

I8 Frustum::ContainsSphere(const vec3<F32>& center, F32 radius) const {
    F32 distance = 0.0f;
    for (const Plane<F32>& frustumPlane : _frustumPlanes){
        distance = frustumPlane.getDistance(center);

        if (distance < -radius)
            return FRUSTUM_OUT;

        if (fabs(distance) < radius)
            return FRUSTUM_INTERSECT;
    }

    return FRUSTUM_IN;
}

I8 Frustum::ContainsBoundingBox(const BoundingBox& bbox) const {
    const vec3<F32> *boxCorners = bbox.getPoints();

    I32 iTotalIn = 0;
    I32 iInCount = 8;
    I32 iPtIn = 1;

    for (const Plane<F32>& frustumPlane : _frustumPlanes){
        iInCount = 8;
        iPtIn = 1;

        for(U8 c = 0; c < 8; ++c) {
            if (frustumPlane.classifyPoint(boxCorners[c]) == Plane<F32>::NEGATIVE_SIDE) {
                iPtIn = 0;
                iInCount--;
            }
        }

        if(iInCount == 0)
            return FRUSTUM_OUT;

        iTotalIn += iPtIn;
    }

    if(iTotalIn == 6)
        return FRUSTUM_IN;

    return FRUSTUM_INTERSECT;
}

void Frustum::Extract(const vec3<F32>& eye){
    _eyePos = eye;

    const mat4<F32>& viewProjectionMatrixCache = GFX_DEVICE.getMatrix(VIEW_PROJECTION_MATRIX);

    Plane<F32>& rightPlane  = _frustumPlanes[0];
    Plane<F32>& leftPlane   = _frustumPlanes[1];
    Plane<F32>& bottomPlane = _frustumPlanes[2];
    Plane<F32>& topPlane    = _frustumPlanes[3];
    Plane<F32>& farPlane    = _frustumPlanes[4];
    Plane<F32>& nearPlane   = _frustumPlanes[5];

    // Right plane
    rightPlane.set(viewProjectionMatrixCache[ 3] - viewProjectionMatrixCache[ 0],
                   viewProjectionMatrixCache[ 7] - viewProjectionMatrixCache[ 4],
                   viewProjectionMatrixCache[11] - viewProjectionMatrixCache[ 8],
                   viewProjectionMatrixCache[15] - viewProjectionMatrixCache[12]);

    rightPlane.normalize();

    // Left plane
    leftPlane.set(viewProjectionMatrixCache[ 3] + viewProjectionMatrixCache[ 0],
                  viewProjectionMatrixCache[ 7] + viewProjectionMatrixCache[ 4],
                  viewProjectionMatrixCache[11] + viewProjectionMatrixCache[ 8],
                  viewProjectionMatrixCache[15] + viewProjectionMatrixCache[12]);

    leftPlane.normalize();

    // Bottom plane
    bottomPlane.set(viewProjectionMatrixCache[ 3] + viewProjectionMatrixCache[ 1],
                    viewProjectionMatrixCache[ 7] + viewProjectionMatrixCache[ 5],
                    viewProjectionMatrixCache[11] + viewProjectionMatrixCache[ 9],
                    viewProjectionMatrixCache[15] + viewProjectionMatrixCache[13]);

    bottomPlane.normalize();

    // Top plane
    topPlane.set(viewProjectionMatrixCache[ 3] - viewProjectionMatrixCache[ 1],
                 viewProjectionMatrixCache[ 7] - viewProjectionMatrixCache[ 5],
                 viewProjectionMatrixCache[11] - viewProjectionMatrixCache[ 9],
                 viewProjectionMatrixCache[15] - viewProjectionMatrixCache[13]);

    topPlane.normalize();

    // Far Plane
    farPlane.set(viewProjectionMatrixCache[ 3] - viewProjectionMatrixCache[ 2],
                 viewProjectionMatrixCache[ 7] - viewProjectionMatrixCache[ 6],
                 viewProjectionMatrixCache[11] - viewProjectionMatrixCache[10],
                 viewProjectionMatrixCache[15] - viewProjectionMatrixCache[14]);

    farPlane.normalize();

    // Near Plane
    nearPlane.set(viewProjectionMatrixCache[ 3] + viewProjectionMatrixCache[ 2],
                  viewProjectionMatrixCache[ 7] + viewProjectionMatrixCache[ 6],
                  viewProjectionMatrixCache[11] + viewProjectionMatrixCache[10],
                  viewProjectionMatrixCache[15] + viewProjectionMatrixCache[14]);
 
    nearPlane.normalize();

    _pointsDirty = true;


}

void Frustum::intersectionPoint(const Plane<F32> & a, const Plane<F32> & b, const Plane<F32> & c, vec3<F32>& outResult){
    outResult.set((a.getDistance() * (Cross(b.getNormal(), c.getNormal()))) + 
                  (b.getDistance() * (Cross(c.getNormal(), a.getNormal()))) + 
                  (c.getDistance() * (Cross(a.getNormal(), b.getNormal()))) / -Dot(a.getNormal(), Cross(b.getNormal(), c.getNormal())));
}

void Frustum::updatePoints(){
    if (!_pointsDirty)
        return;

    const Plane<F32>& rightPlane  = _frustumPlanes[0];
    const Plane<F32>& leftPlane = _frustumPlanes[1];
    const Plane<F32>& bottomPlane = _frustumPlanes[2];
    const Plane<F32>& topPlane = _frustumPlanes[3];
    const Plane<F32>& farPlane = _frustumPlanes[4];
    const Plane<F32>& nearPlane = _frustumPlanes[5];

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

void Frustum::setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes, bool updateGFX) {
    _aspectRatio = aspectRatio;
    _verticalFoV = verticalFoV;
    _zPlanes = zPlanes;
    if (updateGFX){
        GFX_DEVICE.updateProjection();
    }
}
#include "Headers/Frustum.h"

#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

bool Frustum::ContainsPoint(const vec3<F32>& point) const {
   for(I8 p = 0; p < 6; ++p)
      if(_frustumPlanes[p][0] * point.x +
         _frustumPlanes[p][1] * point.y +
         _frustumPlanes[p][2] * point.z +
         _frustumPlanes[p][3] <= 0)
         return false;

   return true;
}

I8 Frustum::ContainsSphere(const vec3<F32>& center, F32 radius) const {
    F32 t = 0.0f;
    for(U8 p = 0; p < 6; ++p) {
        t =	_frustumPlanes[p][0] * center.x +
            _frustumPlanes[p][1] * center.y +
            _frustumPlanes[p][2] * center.z +
            _frustumPlanes[p][3];

        if( t < -radius)
            return FRUSTUM_OUT;

        if(fabs(t) < radius)
            return FRUSTUM_INTERSECT;
    }

    return FRUSTUM_IN;
}

I8 Frustum::ContainsBoundingBox(const BoundingBox& bbox) const {
    const vec3<F32> *tCorners = bbox.getPoints();

    I32 iTotalIn = 0;
    I32 iInCount = 8;
    I32 iPtIn = 1;
    F32 side = 0.0f;

    for(U8 p = 0; p < 6; ++p) {
        iInCount = 8;
        iPtIn = 1;

        for(U8 c = 0; c < 8; ++c) {
            side = _frustumPlanes[p][0] * tCorners[c].x +
                   _frustumPlanes[p][1] * tCorners[c].y +
                   _frustumPlanes[p][2] * tCorners[c].z +
                   _frustumPlanes[p][3];

            if(side < 0) {
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

    GFX_DEVICE.getMatrix(MVP_MATRIX, _modelViewProjectionMatrixCache);

    _frustumPlanes[0][0] = _modelViewProjectionMatrixCache[ 3] - _modelViewProjectionMatrixCache[ 0];
    _frustumPlanes[0][1] = _modelViewProjectionMatrixCache[ 7] - _modelViewProjectionMatrixCache[ 4];
    _frustumPlanes[0][2] = _modelViewProjectionMatrixCache[11] - _modelViewProjectionMatrixCache[ 8];
    _frustumPlanes[0][3] = _modelViewProjectionMatrixCache[15] - _modelViewProjectionMatrixCache[12];

    _frustumPlanes[0].normalize();

    _frustumPlanes[1][0] = _modelViewProjectionMatrixCache[ 3] + _modelViewProjectionMatrixCache[ 0];
    _frustumPlanes[1][1] = _modelViewProjectionMatrixCache[ 7] + _modelViewProjectionMatrixCache[ 4];
    _frustumPlanes[1][2] = _modelViewProjectionMatrixCache[11] + _modelViewProjectionMatrixCache[ 8];
    _frustumPlanes[1][3] = _modelViewProjectionMatrixCache[15] + _modelViewProjectionMatrixCache[12];

    _frustumPlanes[1].normalize();

    _frustumPlanes[2][0] = _modelViewProjectionMatrixCache[ 3] + _modelViewProjectionMatrixCache[ 1];
    _frustumPlanes[2][1] = _modelViewProjectionMatrixCache[ 7] + _modelViewProjectionMatrixCache[ 5];
    _frustumPlanes[2][2] = _modelViewProjectionMatrixCache[11] + _modelViewProjectionMatrixCache[ 9];
    _frustumPlanes[2][3] = _modelViewProjectionMatrixCache[15] + _modelViewProjectionMatrixCache[13];

    _frustumPlanes[2].normalize();

    _frustumPlanes[3][0] = _modelViewProjectionMatrixCache[ 3] - _modelViewProjectionMatrixCache[ 1];
    _frustumPlanes[3][1] = _modelViewProjectionMatrixCache[ 7] - _modelViewProjectionMatrixCache[ 5];
    _frustumPlanes[3][2] = _modelViewProjectionMatrixCache[11] - _modelViewProjectionMatrixCache[ 9];
    _frustumPlanes[3][3] = _modelViewProjectionMatrixCache[15] - _modelViewProjectionMatrixCache[13];

    _frustumPlanes[3].normalize();

    // Far Plane
    _frustumPlanes[4][0] = _modelViewProjectionMatrixCache[ 3] - _modelViewProjectionMatrixCache[ 2];
    _frustumPlanes[4][1] = _modelViewProjectionMatrixCache[ 7] - _modelViewProjectionMatrixCache[ 6];
    _frustumPlanes[4][2] = _modelViewProjectionMatrixCache[11] - _modelViewProjectionMatrixCache[10];
    _frustumPlanes[4][3] = _modelViewProjectionMatrixCache[15] - _modelViewProjectionMatrixCache[14];

    _frustumPlanes[4].normalize();

    // Near Plane
    _frustumPlanes[5][0] = _modelViewProjectionMatrixCache[3]  + _modelViewProjectionMatrixCache[ 2];
    _frustumPlanes[5][1] = _modelViewProjectionMatrixCache[7]  + _modelViewProjectionMatrixCache[ 6];
    _frustumPlanes[5][2] = _modelViewProjectionMatrixCache[11] + _modelViewProjectionMatrixCache[10];
    _frustumPlanes[5][3] = _modelViewProjectionMatrixCache[15] + _modelViewProjectionMatrixCache[14];

    _frustumPlanes[5].normalize();
}
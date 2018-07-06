#include "Headers/Frustum.h"

#include "core.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Math/BoundingVolumes/Headers/BoundingBox.h"

bool Frustum::ContainsPoint(const vec3<F32>& point) const {
   for(I8 p = 0; p < 6; p++)
      if(	_frustumPlanes[p][0] * point.x +
			_frustumPlanes[p][1] * point.y +
			_frustumPlanes[p][2] * point.z +
			_frustumPlanes[p][3] <= 0)
         return false;

   return true;
}

I8 Frustum::ContainsSphere(const vec3<F32>& center, F32 radius) const {
	for(I8 p = 0; p < 6; p++) {
		F32 t =	_frustumPlanes[p][0] * center.x +
				_frustumPlanes[p][1] * center.y +
				_frustumPlanes[p][2] * center.z +
				_frustumPlanes[p][3];

		if( t < -radius)
			return FRUSTUM_OUT;

		if((F32)fabs(t) < radius)
			return FRUSTUM_INTERSECT;
	}
	return FRUSTUM_IN;
}

I8 Frustum::ContainsBoundingBox(const BoundingBox& bbox) const {
	const vec3<F32>& min = bbox.getMin();
	const vec3<F32>& max = bbox.getMax();
	vec3<F32> tCorners[8] = {	vec3<F32>(min.x, min.y, min.z),
							    vec3<F32>(max.x, min.y, min.z),
							    vec3<F32>(min.x, max.y, min.z),
							    vec3<F32>(min.x, min.y, max.z),
							    vec3<F32>(max.x, max.y, min.z),
							    vec3<F32>(min.x, max.y, max.z),
							    vec3<F32>(max.x, min.y, max.z),
							    vec3<F32>(max.x, max.y, max.z)};
	I32 iTotalIn = 0;

	for(I8 p=0; p<6; p++){
		I32 iInCount = 8;
		I32 iPtIn = 1;

		for(I32 c=0; c<8; c++)
		{
			F32 side =	_frustumPlanes[p][0] * tCorners[c].x +
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

    mat4<F32> modelViewProjectionMatrix;

	GFX_DEVICE.getMatrix(MVP_MATRIX, modelViewProjectionMatrix);

	_frustumPlanes[0][0] = modelViewProjectionMatrix[ 3] - modelViewProjectionMatrix[ 0];
	_frustumPlanes[0][1] = modelViewProjectionMatrix[ 7] - modelViewProjectionMatrix[ 4];
	_frustumPlanes[0][2] = modelViewProjectionMatrix[11] - modelViewProjectionMatrix[ 8];
	_frustumPlanes[0][3] = modelViewProjectionMatrix[15] - modelViewProjectionMatrix[12];

    F32 t = square_root_tpl<F32>( _frustumPlanes[0][0] * _frustumPlanes[0][0] +
                                  _frustumPlanes[0][1] * _frustumPlanes[0][1] +
                                  _frustumPlanes[0][2] * _frustumPlanes[0][2] );

	_frustumPlanes[0][0] /= t;
	_frustumPlanes[0][1] /= t;
	_frustumPlanes[0][2] /= t;
	_frustumPlanes[0][3] /= t;

	_frustumPlanes[1][0] = modelViewProjectionMatrix[ 3] + modelViewProjectionMatrix[ 0];
	_frustumPlanes[1][1] = modelViewProjectionMatrix[ 7] + modelViewProjectionMatrix[ 4];
	_frustumPlanes[1][2] = modelViewProjectionMatrix[11] + modelViewProjectionMatrix[ 8];
	_frustumPlanes[1][3] = modelViewProjectionMatrix[15] + modelViewProjectionMatrix[12];

	t = square_root_tpl<F32>( _frustumPlanes[1][0] * _frustumPlanes[1][0] +
                              _frustumPlanes[1][1] * _frustumPlanes[1][1] +
                              _frustumPlanes[1][2] * _frustumPlanes[1][2] );

	_frustumPlanes[1][0] /= t;
	_frustumPlanes[1][1] /= t;
	_frustumPlanes[1][2] /= t;
	_frustumPlanes[1][3] /= t;

	_frustumPlanes[2][0] = modelViewProjectionMatrix[ 3] + modelViewProjectionMatrix[ 1];
	_frustumPlanes[2][1] = modelViewProjectionMatrix[ 7] + modelViewProjectionMatrix[ 5];
	_frustumPlanes[2][2] = modelViewProjectionMatrix[11] + modelViewProjectionMatrix[ 9];
	_frustumPlanes[2][3] = modelViewProjectionMatrix[15] + modelViewProjectionMatrix[13];

	t = square_root_tpl<F32>( _frustumPlanes[2][0] * _frustumPlanes[2][0] +
                              _frustumPlanes[2][1] * _frustumPlanes[2][1] +
                              _frustumPlanes[2][2] * _frustumPlanes[2][2] );

	_frustumPlanes[2][0] /= t;
	_frustumPlanes[2][1] /= t;
	_frustumPlanes[2][2] /= t;
	_frustumPlanes[2][3] /= t;

	_frustumPlanes[3][0] = modelViewProjectionMatrix[ 3] - modelViewProjectionMatrix[ 1];
	_frustumPlanes[3][1] = modelViewProjectionMatrix[ 7] - modelViewProjectionMatrix[ 5];
	_frustumPlanes[3][2] = modelViewProjectionMatrix[11] - modelViewProjectionMatrix[ 9];
	_frustumPlanes[3][3] = modelViewProjectionMatrix[15] - modelViewProjectionMatrix[13];

	t = square_root_tpl<F32>( _frustumPlanes[3][0] * _frustumPlanes[3][0] +
                              _frustumPlanes[3][1] * _frustumPlanes[3][1] +
                              _frustumPlanes[3][2] * _frustumPlanes[3][2] );

	_frustumPlanes[3][0] /= t;
	_frustumPlanes[3][1] /= t;
	_frustumPlanes[3][2] /= t;
	_frustumPlanes[3][3] /= t;

	_frustumPlanes[4][0] = modelViewProjectionMatrix[ 3] - modelViewProjectionMatrix[ 2];
	_frustumPlanes[4][1] = modelViewProjectionMatrix[ 7] - modelViewProjectionMatrix[ 6];
	_frustumPlanes[4][2] = modelViewProjectionMatrix[11] - modelViewProjectionMatrix[10];
	_frustumPlanes[4][3] = modelViewProjectionMatrix[15] - modelViewProjectionMatrix[14];

	t = square_root_tpl<F32>( _frustumPlanes[4][0] * _frustumPlanes[4][0] +
                              _frustumPlanes[4][1] * _frustumPlanes[4][1] +
                              _frustumPlanes[4][2] * _frustumPlanes[4][2] );

	_frustumPlanes[4][0] /= t;
	_frustumPlanes[4][1] /= t;
	_frustumPlanes[4][2] /= t;
	_frustumPlanes[4][3] /= t;

	_frustumPlanes[5][0] = modelViewProjectionMatrix[3]  + modelViewProjectionMatrix[ 2];
	_frustumPlanes[5][1] = modelViewProjectionMatrix[7]  + modelViewProjectionMatrix[ 6];
	_frustumPlanes[5][2] = modelViewProjectionMatrix[11] + modelViewProjectionMatrix[10];
	_frustumPlanes[5][3] = modelViewProjectionMatrix[15] + modelViewProjectionMatrix[14];

	t = square_root_tpl<F32>( _frustumPlanes[5][0] * _frustumPlanes[5][0] +
                              _frustumPlanes[5][1] * _frustumPlanes[5][1] +
                              _frustumPlanes[5][2] * _frustumPlanes[5][2] );

	_frustumPlanes[5][0] /= t;
	_frustumPlanes[5][1] /= t;
	_frustumPlanes[5][2] /= t;
	_frustumPlanes[5][3] /= t;
}
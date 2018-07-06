#include "Frustum.h"
#include "resource.h"
#include "Hardware/Video/GFXDevice.h"
#include "Utility/Headers/BoundingBox.h"

bool Frustum::ContainsPoint(const vec3& point) const
{
   for(I8 p = 0; p < 6; p++)	
      if(	_frustumPlanes[p][0] * point.x +
			_frustumPlanes[p][1] * point.y +
			_frustumPlanes[p][2] * point.z +
			_frustumPlanes[p][3] <= 0)
         return false;

   return true;
}

I8 Frustum::ContainsSphere(const vec3& center, F32 radius) const
{
	for(I8 p = 0; p < 6; p++)
	{
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


I8 Frustum::ContainsBoundingBox(BoundingBox& bbox) const
{
	vec3& min = bbox.getMin();
	vec3& max = bbox.getMax();
	vec3 tCorners[8] = {	vec3(min.x, min.y, min.z),
							vec3(max.x, min.y, min.z),
							vec3(min.x, max.y, min.z),
							vec3(min.x, min.y, max.z),
							vec3(max.x, max.y, min.z),
							vec3(min.x, max.y, max.z),
							vec3(max.x, min.y, max.z),
							vec3(max.x, max.y, max.z)};
	I32 iTotalIn = 0;

	for(I32 p=0; p<6; p++)
	{
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



void Frustum::Extract(const vec3& eye)
{
	_eyePos = eye;

	GFXDevice& gfx = GFXDevice::getInstance();
	gfx.getModelViewMatrix(_modelViewMatrix);
	gfx.getProjectionMatrix(_projectionMatrix);
	_modelViewMatrix.inverse(_modelViewMatrixInv);						
	
	F32 t;

	
	_modelViewProjectionMatrix = _projectionMatrix * _modelViewMatrix;
	_inverseModelViewProjectionMatrix = _projectionMatrix * _modelViewMatrixInv;


	
	_frustumPlanes[0][0] = _modelViewProjectionMatrix[ 3] - _modelViewProjectionMatrix[ 0];
	_frustumPlanes[0][1] = _modelViewProjectionMatrix[ 7] - _modelViewProjectionMatrix[ 4];
	_frustumPlanes[0][2] = _modelViewProjectionMatrix[11] - _modelViewProjectionMatrix[ 8];
	_frustumPlanes[0][3] = _modelViewProjectionMatrix[15] - _modelViewProjectionMatrix[12];

	
	t = Util::square_root( _frustumPlanes[0][0] * _frustumPlanes[0][0] + _frustumPlanes[0][1] * _frustumPlanes[0][1] + _frustumPlanes[0][2] * _frustumPlanes[0][2] );
	_frustumPlanes[0][0] /= t;
	_frustumPlanes[0][1] /= t;
	_frustumPlanes[0][2] /= t;
	_frustumPlanes[0][3] /= t;

	
	_frustumPlanes[1][0] = _modelViewProjectionMatrix[ 3] + _modelViewProjectionMatrix[ 0];
	_frustumPlanes[1][1] = _modelViewProjectionMatrix[ 7] + _modelViewProjectionMatrix[ 4];
	_frustumPlanes[1][2] = _modelViewProjectionMatrix[11] + _modelViewProjectionMatrix[ 8];
	_frustumPlanes[1][3] = _modelViewProjectionMatrix[15] + _modelViewProjectionMatrix[12];

	
	t = Util::square_root( _frustumPlanes[1][0] * _frustumPlanes[1][0] + _frustumPlanes[1][1] * _frustumPlanes[1][1] + _frustumPlanes[1][2] * _frustumPlanes[1][2] );
	_frustumPlanes[1][0] /= t;
	_frustumPlanes[1][1] /= t;
	_frustumPlanes[1][2] /= t;
	_frustumPlanes[1][3] /= t;


	_frustumPlanes[2][0] = _modelViewProjectionMatrix[ 3] + _modelViewProjectionMatrix[ 1];
	_frustumPlanes[2][1] = _modelViewProjectionMatrix[ 7] + _modelViewProjectionMatrix[ 5];
	_frustumPlanes[2][2] = _modelViewProjectionMatrix[11] + _modelViewProjectionMatrix[ 9];
	_frustumPlanes[2][3] = _modelViewProjectionMatrix[15] + _modelViewProjectionMatrix[13];

	
	t = Util::square_root( _frustumPlanes[2][0] * _frustumPlanes[2][0] + _frustumPlanes[2][1] * _frustumPlanes[2][1] + _frustumPlanes[2][2] * _frustumPlanes[2][2] );
	_frustumPlanes[2][0] /= t;
	_frustumPlanes[2][1] /= t;
	_frustumPlanes[2][2] /= t;
	_frustumPlanes[2][3] /= t;

	
	_frustumPlanes[3][0] = _modelViewProjectionMatrix[ 3] - _modelViewProjectionMatrix[ 1];
	_frustumPlanes[3][1] = _modelViewProjectionMatrix[ 7] - _modelViewProjectionMatrix[ 5];
	_frustumPlanes[3][2] = _modelViewProjectionMatrix[11] - _modelViewProjectionMatrix[ 9];
	_frustumPlanes[3][3] = _modelViewProjectionMatrix[15] - _modelViewProjectionMatrix[13];

	
	t = Util::square_root( _frustumPlanes[3][0] * _frustumPlanes[3][0] + _frustumPlanes[3][1] * _frustumPlanes[3][1] + _frustumPlanes[3][2] * _frustumPlanes[3][2] );
	_frustumPlanes[3][0] /= t;
	_frustumPlanes[3][1] /= t;
	_frustumPlanes[3][2] /= t;
	_frustumPlanes[3][3] /= t;

	
	_frustumPlanes[4][0] = _modelViewProjectionMatrix[ 3] - _modelViewProjectionMatrix[ 2];
	_frustumPlanes[4][1] = _modelViewProjectionMatrix[ 7] - _modelViewProjectionMatrix[ 6];
	_frustumPlanes[4][2] = _modelViewProjectionMatrix[11] - _modelViewProjectionMatrix[10];
	_frustumPlanes[4][3] = _modelViewProjectionMatrix[15] - _modelViewProjectionMatrix[14];

	
	t = Util::square_root( _frustumPlanes[4][0] * _frustumPlanes[4][0] + _frustumPlanes[4][1] * _frustumPlanes[4][1] + _frustumPlanes[4][2] * _frustumPlanes[4][2] );
	_frustumPlanes[4][0] /= t;
	_frustumPlanes[4][1] /= t;
	_frustumPlanes[4][2] /= t;
	_frustumPlanes[4][3] /= t;

	
	_frustumPlanes[5][0] = _modelViewProjectionMatrix[3]  + _modelViewProjectionMatrix[ 2];
	_frustumPlanes[5][1] = _modelViewProjectionMatrix[ 7] + _modelViewProjectionMatrix[ 6];
	_frustumPlanes[5][2] = _modelViewProjectionMatrix[11] + _modelViewProjectionMatrix[10];
	_frustumPlanes[5][3] = _modelViewProjectionMatrix[15] + _modelViewProjectionMatrix[14];

	
	t = Util::square_root( _frustumPlanes[5][0] * _frustumPlanes[5][0] + _frustumPlanes[5][1] * _frustumPlanes[5][1] + _frustumPlanes[5][2] * _frustumPlanes[5][2] );
	_frustumPlanes[5][0] /= t;
	_frustumPlanes[5][1] /= t;
	_frustumPlanes[5][2] /= t;
	_frustumPlanes[5][3] /= t;


}



#include "Frustum.h"
#include "resource.h"
#include "Hardware/Video/GFXDevice.h"

bool Frustum::ContainsPoint(const vec3& point) const
{
   for(int p = 0; p < 6; p++)	
      if(	m_tFrustumPlanes[p][0] * point.x +
			m_tFrustumPlanes[p][1] * point.y +
			m_tFrustumPlanes[p][2] * point.z +
			m_tFrustumPlanes[p][3] <= 0)
         return false;

   return true;
}

int Frustum::ContainsSphere(const vec3& center, float radius) const
{
	for(int p = 0; p < 6; p++)
	{
		F32 t =	m_tFrustumPlanes[p][0] * center.x +
				m_tFrustumPlanes[p][1] * center.y +
				m_tFrustumPlanes[p][2] * center.z +
				m_tFrustumPlanes[p][3];

		if( t < -radius)
			return FRUSTUM_OUT;

		if((F32)fabs(t) < radius)
			return FRUSTUM_INTERSECT;
	}
	return FRUSTUM_IN;
}


int Frustum::ContainsBoundingBox(const BoundingBox& bbox) const
{
	vec3 tCorners[8] = {	vec3(bbox.min.x, bbox.min.y, bbox.min.z),
							vec3(bbox.max.x, bbox.min.y, bbox.min.z),
							vec3(bbox.min.x, bbox.max.y, bbox.min.z),
							vec3(bbox.min.x, bbox.min.y, bbox.max.z),
							vec3(bbox.max.x, bbox.max.y, bbox.min.z),
							vec3(bbox.min.x, bbox.max.y, bbox.max.z),
							vec3(bbox.max.x, bbox.min.y, bbox.max.z),
							vec3(bbox.max.x, bbox.max.y, bbox.max.z)	};
	int iTotalIn = 0;

	for(int p=0; p<6; p++)
	{
		int iInCount = 8;
		int iPtIn = 1;

		for(int c=0; c<8; c++)
		{
			F32 side =	m_tFrustumPlanes[p][0] * tCorners[c].x +
						m_tFrustumPlanes[p][1] * tCorners[c].y +
						m_tFrustumPlanes[p][2] * tCorners[c].z +
						m_tFrustumPlanes[p][3];
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
	m_EyePos = eye;


	m_mtxMV = GFXDevice::getInstance().getModelViewMatrix();
	m_mtxProj = GFXDevice::getInstance().getProjectionMatrix();
	m_mtxMV.inverse(m_mtxMVinv);						
	
	F32 t;

	
	m_mtxMVProj = m_mtxProj * m_mtxMV;


	
	m_tFrustumPlanes[0][0] = m_mtxMVProj[ 3] - m_mtxMVProj[ 0];
	m_tFrustumPlanes[0][1] = m_mtxMVProj[ 7] - m_mtxMVProj[ 4];
	m_tFrustumPlanes[0][2] = m_mtxMVProj[11] - m_mtxMVProj[ 8];
	m_tFrustumPlanes[0][3] = m_mtxMVProj[15] - m_mtxMVProj[12];

	
	t = sqrt( m_tFrustumPlanes[0][0] * m_tFrustumPlanes[0][0] + m_tFrustumPlanes[0][1] * m_tFrustumPlanes[0][1] + m_tFrustumPlanes[0][2] * m_tFrustumPlanes[0][2] );
	m_tFrustumPlanes[0][0] /= t;
	m_tFrustumPlanes[0][1] /= t;
	m_tFrustumPlanes[0][2] /= t;
	m_tFrustumPlanes[0][3] /= t;

	
	m_tFrustumPlanes[1][0] = m_mtxMVProj[ 3] + m_mtxMVProj[ 0];
	m_tFrustumPlanes[1][1] = m_mtxMVProj[ 7] + m_mtxMVProj[ 4];
	m_tFrustumPlanes[1][2] = m_mtxMVProj[11] + m_mtxMVProj[ 8];
	m_tFrustumPlanes[1][3] = m_mtxMVProj[15] + m_mtxMVProj[12];

	
	t = sqrt( m_tFrustumPlanes[1][0] * m_tFrustumPlanes[1][0] + m_tFrustumPlanes[1][1] * m_tFrustumPlanes[1][1] + m_tFrustumPlanes[1][2] * m_tFrustumPlanes[1][2] );
	m_tFrustumPlanes[1][0] /= t;
	m_tFrustumPlanes[1][1] /= t;
	m_tFrustumPlanes[1][2] /= t;
	m_tFrustumPlanes[1][3] /= t;


	m_tFrustumPlanes[2][0] = m_mtxMVProj[ 3] + m_mtxMVProj[ 1];
	m_tFrustumPlanes[2][1] = m_mtxMVProj[ 7] + m_mtxMVProj[ 5];
	m_tFrustumPlanes[2][2] = m_mtxMVProj[11] + m_mtxMVProj[ 9];
	m_tFrustumPlanes[2][3] = m_mtxMVProj[15] + m_mtxMVProj[13];

	
	t = sqrt( m_tFrustumPlanes[2][0] * m_tFrustumPlanes[2][0] + m_tFrustumPlanes[2][1] * m_tFrustumPlanes[2][1] + m_tFrustumPlanes[2][2] * m_tFrustumPlanes[2][2] );
	m_tFrustumPlanes[2][0] /= t;
	m_tFrustumPlanes[2][1] /= t;
	m_tFrustumPlanes[2][2] /= t;
	m_tFrustumPlanes[2][3] /= t;

	
	m_tFrustumPlanes[3][0] = m_mtxMVProj[ 3] - m_mtxMVProj[ 1];
	m_tFrustumPlanes[3][1] = m_mtxMVProj[ 7] - m_mtxMVProj[ 5];
	m_tFrustumPlanes[3][2] = m_mtxMVProj[11] - m_mtxMVProj[ 9];
	m_tFrustumPlanes[3][3] = m_mtxMVProj[15] - m_mtxMVProj[13];

	
	t = sqrt( m_tFrustumPlanes[3][0] * m_tFrustumPlanes[3][0] + m_tFrustumPlanes[3][1] * m_tFrustumPlanes[3][1] + m_tFrustumPlanes[3][2] * m_tFrustumPlanes[3][2] );
	m_tFrustumPlanes[3][0] /= t;
	m_tFrustumPlanes[3][1] /= t;
	m_tFrustumPlanes[3][2] /= t;
	m_tFrustumPlanes[3][3] /= t;

	
	m_tFrustumPlanes[4][0] = m_mtxMVProj[ 3] - m_mtxMVProj[ 2];
	m_tFrustumPlanes[4][1] = m_mtxMVProj[ 7] - m_mtxMVProj[ 6];
	m_tFrustumPlanes[4][2] = m_mtxMVProj[11] - m_mtxMVProj[10];
	m_tFrustumPlanes[4][3] = m_mtxMVProj[15] - m_mtxMVProj[14];

	
	t = sqrt( m_tFrustumPlanes[4][0] * m_tFrustumPlanes[4][0] + m_tFrustumPlanes[4][1] * m_tFrustumPlanes[4][1] + m_tFrustumPlanes[4][2] * m_tFrustumPlanes[4][2] );
	m_tFrustumPlanes[4][0] /= t;
	m_tFrustumPlanes[4][1] /= t;
	m_tFrustumPlanes[4][2] /= t;
	m_tFrustumPlanes[4][3] /= t;

	
	m_tFrustumPlanes[5][0] = m_mtxMVProj[3] + m_mtxMVProj[ 2];
	m_tFrustumPlanes[5][1] = m_mtxMVProj[ 7] + m_mtxMVProj[ 6];
	m_tFrustumPlanes[5][2] = m_mtxMVProj[11] + m_mtxMVProj[10];
	m_tFrustumPlanes[5][3] = m_mtxMVProj[15] + m_mtxMVProj[14];

	
	t = sqrt( m_tFrustumPlanes[5][0] * m_tFrustumPlanes[5][0] + m_tFrustumPlanes[5][1] * m_tFrustumPlanes[5][1] + m_tFrustumPlanes[5][2] * m_tFrustumPlanes[5][2] );
	m_tFrustumPlanes[5][0] /= t;
	m_tFrustumPlanes[5][1] /= t;
	m_tFrustumPlanes[5][2] /= t;
	m_tFrustumPlanes[5][3] /= t;


}



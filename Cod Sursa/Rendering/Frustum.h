#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include "Utility/Headers/Singleton.h"
#include "Utility/Headers/BoundingBox.h"

#define FRUSTUM_OUT			0
#define FRUSTUM_IN			1
#define FRUSTUM_INTERSECT	2

SINGLETON_BEGIN( Frustum )

public:
	void Extract(const vec3& eye);

	bool ContainsPoint(const vec3& point) const;
	int ContainsBoundingBox(const BoundingBox& bbox) const;
	int ContainsSphere(const vec3& center, float radius) const;

//	vec3 Intersect2D(int id1, int id2) const;

	vec3& getEyePos()					{return m_EyePos;}
	mat4& getModelviewMatrix()			{return m_mtxMV;}
	mat4& getModelviewInvMatrix()		{return m_mtxMVinv;}
	mat4& getProjectionMatrix()			{return m_mtxProj;}

private:
	vec3	m_EyePos;
	vec4	m_tFrustumPlanes[6];	
	mat4	m_mtxMV, m_mtxMVinv;	// Matricea de Modelview
	mat4	m_mtxProj; 				// Matricea de Projection
	mat4	m_mtxMVProj;			// Matricea Proiectie * Modelview

SINGLETON_END()

#endif

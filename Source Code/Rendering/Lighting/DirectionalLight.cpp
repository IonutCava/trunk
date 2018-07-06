#include "Headers/DirectionalLight.h"
#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/GFXDevice.h" 
#include "ShadowMapping/Headers/ShadowMap.h"
#include "Core/Resources/Headers/ResourceCache.h"
DirectionalLight::DirectionalLight(U8 slot) : Light(slot,-1,LIGHT_TYPE_DIRECTIONAL)
{
	vec2<F32> angle = vec2<F32>(0.0f, RADIANS(45.0f));
	_position = vec4<F32>(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),-sinf(angle.x) * sinf(angle.y),	0.0f );
	_splitWeight = 0.75f;
	_splitCount = 3;
	_lightOrtho[0] = 5.0;
	_lightOrtho[1] = 10.0; 
	_lightOrtho[2] = 50.0;
};

DirectionalLight::~DirectionalLight()
{
}

/// updateSplitDist computes the near and far distances for every frustum slice
/// in camera eye space - that is, at what distance does a slice start and end
/// Compute the 8 corner points of the current view frustum
void DirectionalLight::updateFrustumPoints(frustum &f, vec3<F32> &center, vec3<F32> &viewDir){
	vec3<F32> up(0.0f, 1.0f, 0.0f);
	vec3<F32> viewDir_temp = viewDir;
	viewDir_temp.cross(up);
	vec3<F32> right = viewDir_temp;
	right.normalize();
	vec3<F32> right_temp = right;
	vec3<F32> fc = center + viewDir*f.farPlane;
	vec3<F32> nc = center + viewDir*f.nearPlane;

	
	right_temp.cross(viewDir);
	up = right_temp;
	up.normalize();

	/// these heights and widths are half the heights and widths of
	/// the near and far plane rectangles
	F32 near_height = tan(f.fov/2.0f) * f.nearPlane;
	F32 near_width = near_height * f.ratio;
	F32 far_height = tan(f.fov/2.0f) * f.farPlane;
	F32 far_width = far_height * f.ratio;

	f.point[0] = nc - up*near_height - right*near_width;
	f.point[1] = nc + up*near_height - right*near_width;
	f.point[2] = nc + up*near_height + right*near_width;
	f.point[3] = nc - up*near_height + right*near_width;

	f.point[4] = fc - up*far_height - right*far_width;
	f.point[5] = fc + up*far_height - right*far_width;
	f.point[6] = fc + up*far_height + right*far_width;
	f.point[7] = fc - up*far_height + right*far_width;
}

void DirectionalLight::updateSplitDist(frustum f[3], F32 nearPlane, F32 farPlane){
	F32 lambda = _splitWeight;
	F32 ratio = farPlane/nearPlane;
	f[0].nearPlane = nearPlane;

	for(U8 i=1; i<_splitCount; i++){
		F32 si = i / (F32)_splitCount;

		f[i].nearPlane = lambda*(nearPlane*powf(ratio, si)) + (1-lambda)*(nearPlane + (farPlane - nearPlane)*si);
		f[i-1].farPlane = f[i].nearPlane * 1.005f;
	}
	f[_splitCount-1].farPlane = farPlane;
}


void DirectionalLight::setCameraToLightView(const vec3<F32>& eyePos){
	_eyePos = eyePos;
	///Set the virtual light position 500 units above our eye position
	///This is one example why we have different setup functions for each light type
	///This isn't valid for omni or spot
	_lightPos = vec3<F32>(_eyePos.x - 500*_position.x,	
					      _eyePos.y - 500*_position.y,
					      _eyePos.z - 500*_position.z);
	  
	///Tell our rendering API to move the camera
	GFX_DEVICE.lookAt(_lightPos,    //the light's virtual position
					   _eyePos);    //the light's target
}

void DirectionalLight::renderFromLightView(U8 depthPass){

	///ToDo: Near and far planes. Should optimize later! -Ionut
	_zPlanes = Frustum::getInstance().getZPlanes();
	///Set the current projection depending on the current depth pass
	GFX_DEVICE.setOrthoProjection(vec4<F32>(-_lightOrtho[depthPass], 
											 _lightOrtho[depthPass],
											-_lightOrtho[depthPass], 
											 _lightOrtho[depthPass]),
											 _zPlanes);
	Frustum::getInstance().Extract(_eyePos - _lightPos);
}


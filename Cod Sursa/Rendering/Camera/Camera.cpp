#include "Camera.h"
#include "Rendering/Frustum.h"
#include "Hardware/Video/GFXDevice.h"
#include "Managers/SceneManager.h"

void Camera::SaveCamera()
{
	tSaveVectors[0] = vEye;
	tSaveVectors[1] = vCenter;
	tSaveVectors[2] = vViewDir;
	tSaveVectors[3] = vLeftDir;
	tSaveVectors[4] = vUp;

	tSaveFloats[0] = fAngleX;
	tSaveFloats[1] = fAngleY;

	bSaved = true;
}

void Camera::RestoreCamera()
{
	if(bSaved) {
		vEye = tSaveVectors[0];
		vCenter = tSaveVectors[1];
		vViewDir = tSaveVectors[2];
		vLeftDir = tSaveVectors[3];
		vUp = tSaveVectors[4];

		fAngleX = tSaveFloats[0];
		fAngleY = tSaveFloats[1];
	}

	bSaved = false;
}


Camera::Camera() {

	fAngleX	=	3.0f;
	fAngleY	=	M_PI/2;

	vUp			= vec3(0.0f, 1.0f, 0.0f);
	vEye		= vec3(0.0f, 0.0f, 0.0f);
	bSaved = false;
	Refresh();
}


void Camera::Refresh()
{	

	switch(eType) {
	case FREE_FLY:
		vViewDir.x = cosf(fAngleX) * sinf(fAngleY);
		vViewDir.y = cosf(fAngleY);
		vViewDir.z = sinf(fAngleX) * sinf(fAngleY);
		vCenter = vEye + vViewDir;
		vLeftDir.cross(vUp, vViewDir);
		vLeftDir.normalize();
		break;

	case SCRIPTED:
		vViewDir = vCenter - vEye;
		vViewDir.normalize();
		vLeftDir.cross(vUp, vViewDir);
		vLeftDir.normalize();
		break;
	}
}


void Camera::RenderLookAt(bool inverty, F32 planey) {

	vec3 eye = vEye, center = vCenter, up = vUp;
	
	if(inverty)
	{
		eye.y = 2.0f*planey-vEye.y;
		center.y = 2.0f*planey-vCenter.y;
		vUp *= -1;
	}
	GFXDevice::getInstance().lookAt(eye,center,up);

	Frustum::getInstance().Extract(vEye);
}




void Camera::PlayerMoveForward(F32 factor)	{	
	vEye += vViewDir * factor;

	Refresh();
}

void Camera::TranslateForward(F32 factor)	{	
	vEye += vViewDir * factor;

	Refresh();
}

void Camera::PlayerMoveStrafe(F32 factor)	{
	vEye += vLeftDir * factor;

	Refresh();
}

void Camera::TranslateStrafe(F32 factor)	{
	vEye += vLeftDir * factor;

	Refresh();
}


void Camera::MoveAnaglyph(F32 factor)
{
	vEye += vLeftDir * factor;
	vCenter += vLeftDir * factor;
}

void Camera::RenderLookAtToCubeMap(const vec3& eye, U32 nFace)
{
	assert(nFace < 6);

	vec3 TabCenter[6] = {	vec3(eye.x+1.0f,	eye.y,		eye.z),
							vec3(eye.x-1.0f,	eye.y,		eye.z),

							vec3(eye.x,			eye.y+1.0f,	eye.z),
							vec3(eye.x,			eye.y-1.0f,	eye.z),

							vec3(eye.x,			eye.y,		eye.z+1.0f),
							vec3(eye.x,			eye.y,		eye.z-1.0f) };


	static vec3 TabUp[6] = {	vec3(0.0f,	-1.0f,	0.0f),
								vec3(0.0f,	-1.0f,	0.0f),

								vec3(0.0f,	0.0f,	1.0f),
								vec3(0.0f,	0.0f,	-1.0f),

								vec3(0.0f,	-1.0f,	0.0f),
								vec3(0.0f,	-1.0f,	0.0f) };

	setEye( eye );

	GFXDevice::getInstance().enable_MODELVIEW();
	GFXDevice::getInstance().loadIdentityMatrix();
	GFXDevice::getInstance().lookAt(eye,TabCenter[nFace],TabUp[nFace]);
	Frustum::getInstance().Extract(eye);
}


#include "Camera.h"
#include "Rendering/Frustum.h"
#include "Hardware/Video/GFXDevice.h"

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
	vPrevEye = vEye;
	eType = FREE;
	bSaved = false;
	//terHeightMin = Engine::getInstance().getTerrain()->getBoundingBox().min.y;
	Refresh();
}


void Camera::Refresh()
{
	//vec2 terrainDim = Engine::getInstance().getTerrain()->getDimensions();
	//F32 terrainCurrentY = Engine::getInstance().getTerrain()->getPosition(vEye.x/terrainDim.x,vEye.z/terrainDim.y).y;
	switch(eType) {
	case FREE:
		vViewDir.x = cosf(fAngleX) * sinf(fAngleY);
		vViewDir.y = cosf(fAngleY);
		vViewDir.z = sinf(fAngleX) * sinf(fAngleY);
		vCenter = vEye + vViewDir;
	/*	if (vEye.y <= terHeightMin)
			vEye.y += (terHeightMin - vEye.y)+3;
		if (vEye.y <= terrainCurrentY)
			vEye.y += (terrainCurrentY - vEye.y)+3;*/
		vLeftDir.cross(vUp, vViewDir);
		vLeftDir.normalize();
		break;

	case DRIVEN:
		vViewDir = vCenter - vEye;
		vViewDir.normalize();
		vLeftDir.cross(vUp, vViewDir);
		vLeftDir.normalize();
		break;
	}

}



void Camera::RenderLookAt(bool inverty, F32 planey) {

	if(inverty)
		gluLookAt(	vEye.x,		2.0f*planey-vEye.y,		vEye.z,
					vCenter.x,	2.0f*planey-vCenter.y,	vCenter.z,
					-vUp.x,		-vUp.y,					-vUp.z	);
	else

		gluLookAt(	vEye.x,		vEye.y,		vEye.z,
					vCenter.x,	vCenter.y,	vCenter.z,
					vUp.x,		vUp.y,		vUp.z	);
	
	Frustum::getInstance().Extract(vEye);
}




void Camera::PlayerMoveForward(F32 factor)	{	
	vPrevEye = vEye;
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
	gluLookAt(	eye.x,						eye.y,						eye.z,
				TabCenter[nFace].x,	TabCenter[nFace].y,	TabCenter[nFace].z,
				TabUp[nFace].x,				TabUp[nFace].y,				TabUp[nFace].z		);

	Frustum::getInstance().Extract(eye);
}


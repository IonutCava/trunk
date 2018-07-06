#include "Headers/Camera.h"

#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"

void Camera::SaveCamera(){

	tSaveVectors[0] = vEye;
	tSaveVectors[1] = vCenter;
	tSaveVectors[2] = vViewDir;
	tSaveVectors[3] = vLeftDir;
	tSaveVectors[4] = vUp;

	tSaveFloats[0] = fAngleX;
	tSaveFloats[1] = fAngleY;

	bSaved = true;
}

void Camera::RestoreCamera() {

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


Camera::Camera() : Resource() {

	fAngleX	=	3.0f;
	fAngleY	=	M_PI/2;

	vUp			= vec3<F32>(0.0f, 1.0f, 0.0f);
	vEye		= vec3<F32>(0.0f, 0.0f, 0.0f);
	bSaved = false;
	Refresh();
}


void Camera::Refresh() {	

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

///Tell the rendering API to set up our desired PoV
void Camera::RenderLookAt(bool invertx, bool inverty, F32 planey) {
	///Tell the Rendering API to draw from our desired PoV
	if(inverty){							 
		///If we need to flip the camera upside down (ex: for reflections)
		GFX_DEVICE.lookAt(vec3<F32>(vEye.x,2.0f*planey-vEye.y,vEye.z),
						  vec3<F32>(vCenter.x,2.0f*planey-vCenter.y,vCenter.z),
						  vec3<F32>(-vUp.x,-vUp.y,-vUp.z),invertx);
	}else{
		GFX_DEVICE.lookAt(vEye,vCenter,vUp);
	}
	///Extract the frustum associated with our current PoV
	Frustum::getInstance().Extract(vEye);
	///Inform all listeners of a new event
    updateListeners();
}

void Camera::MoveForward(F32 factor)	{	
	vEye += vViewDir * factor;

	Refresh();
}

void Camera::TranslateForward(F32 factor)	{	
	vEye += vViewDir * factor;

	Refresh();
}

void Camera::MoveStrafe(F32 factor)	{
	vEye += vLeftDir * factor;

	Refresh();
}

void Camera::TranslateStrafe(F32 factor)	{
	vEye += vLeftDir * factor;

	Refresh();
}


void Camera::MoveAnaglyph(F32 factor){

	vEye += vLeftDir * factor;
	vCenter += vLeftDir * factor;
}

void Camera::RenderLookAtToCubeMap(const vec3<F32>& eye, U8 nFace){

	assert(nFace < 6);
	///Get the center and up vectors for each cube face
	vec3<F32> TabCenter[6] = {	vec3<F32>(eye.x+1.0f,	eye.y,		eye.z),
							    vec3<F32>(eye.x-1.0f,	eye.y,		eye.z),
							    vec3<F32>(eye.x,		eye.y+1.0f,	eye.z),
							    vec3<F32>(eye.x,		eye.y-1.0f,	eye.z),
      							vec3<F32>(eye.x,		eye.y,		eye.z+1.0f),
							    vec3<F32>(eye.x,		eye.y,		eye.z-1.0f) };


	static vec3<F32> TabUp[6] = {	vec3<F32>(0.0f,	-1.0f,	0.0f),
									vec3<F32>(0.0f,	-1.0f,	0.0f),

									vec3<F32>(0.0f,	0.0f,	1.0f),
									vec3<F32>(0.0f,	0.0f,	-1.0f),

									vec3<F32>(0.0f,	-1.0f,	0.0f),
									vec3<F32>(0.0f,	-1.0f,	0.0f) };
	///Set our eye position
	setEye( eye );
	///Set our Rendering API to render the desired face
	GFX_DEVICE.lookAt(eye,TabCenter[nFace],TabUp[nFace]);
	///Extract the view frustum associated with this face
	Frustum::getInstance().Extract(eye);
	///Inform all listeners of a new event
    updateListeners();
}

void Camera::updateListeners(){
	for_each(boost::function0<void > listener, _listeners){
		listener();
	}
}
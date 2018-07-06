#include "Headers/Camera.h"

#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"

void Camera::SaveCamera(){

	_savedVectors[0] = _eye;
	_savedVectors[1] = _center;
	_savedVectors[2] = _view;
	_savedVectors[3] = _left;
	_savedVectors[4] = _up;

	_savedFloats[0] = _angleX;
	_savedFloats[1] = _angleY;

	_saved = true;
}

void Camera::RestoreCamera() {

	if(_saved) {
		_eye    = _savedVectors[0];
		_center = _savedVectors[1];
		_view   = _savedVectors[2];
		_left   = _savedVectors[3];
		_up     = _savedVectors[4];

		_angleX = _savedFloats[0];
		_angleY = _savedFloats[1];
	}

	_saved = false;
}


Camera::Camera(CameraType type) : Resource(),
				   _saved(false),
				   _angleX(3.0f),
				   _angleY(M_PI/2),
				   _type(type)
{
	_up			= vec3<F32>(0.0f, 1.0f, 0.0f);
	_eye		= vec3<F32>(0.0f, 0.0f, 0.0f);
	_saved = false;
	Refresh();
}


void Camera::Refresh() {	

	switch(_type) {
	case FREE_FLY:
		_view.x = cosf(_angleX) * sinf(_angleY);
		_view.y = cosf(_angleY);
		_view.z = sinf(_angleX) * sinf(_angleY);
		_center = _eye + _view;
		_left.cross(_up, _view);
		_left.normalize();
		break;

	case SCRIPTED:
		_view = _center - _eye;
		_view.normalize();
		_left.cross(_up, _view);
		_left.normalize();
		break;
	}
}

void Camera::MoveForward(F32 factor)	{	
	_eye += _view * factor;
	Refresh();
}

void Camera::TranslateForward(F32 factor)	{	
	_eye += _view * factor;
	Refresh();
}

void Camera::MoveStrafe(F32 factor)	{
	_eye += _left * factor;
	Refresh();
}

void Camera::TranslateStrafe(F32 factor)	{
	_eye += _left * factor;
	Refresh();
}


void Camera::MoveAnaglyph(F32 factor){
	_eye += _left * factor;
	_center += _left * factor;
}

///Tell the rendering API to set up our desired PoV
void Camera::RenderLookAt(bool invertx, bool inverty, F32 planey) {
	///Tell the Rendering API to draw from our desired PoV
	if(inverty){							 
		///If we need to flip the camera upside down (ex: for reflections)
		GFX_DEVICE.lookAt(vec3<F32>(_eye.x,2.0f*planey-_eye.y,_eye.z),
						  vec3<F32>(_center.x,2.0f*planey-_center.y,_center.z),
						  vec3<F32>(-_up.x,-_up.y,-_up.z),invertx);
	}else{
		GFX_DEVICE.lookAt(_eye,_center,_up);
	}
	///Extract the frustum associated with our current PoV
	Frustum::getInstance().Extract(_eye);
	///Inform all listeners of a new event
    updateListeners();
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
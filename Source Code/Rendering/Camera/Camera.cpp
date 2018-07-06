#include "Headers/Camera.h"

#include "Rendering/Headers/Frustum.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"

Camera::Camera(const CameraType& type, const vec3<F32>& eye) :
                                         Resource(),
                                         _saved(false),
                                         _viewMatrixDirty(true),
                                         _mouseSensitivity(1.0f),
                                         _moveSpeedFactor(1.0f),
                                         _turnSpeedFactor(0.1f),
                                         _cameraMoveSpeed(1.0f),
                                         _cameraTurnSpeed(1.0f),
                                         _camIOD(0.5f),
                                         _type(type)
{
    _savedFloats[0] = 0.0f;
    _savedFloats[1] = 0.0f;
    _savedFloats[2] = 0.0f;
    _savedFloats[3] = 0.0f;
    _eye.set(eye);
    _xAxis.set(WORLD_X_AXIS);
    _yAxis.set(WORLD_Y_AXIS);
    _zAxis.set(WORLD_Z_AXIS);
    _viewDir.set(WORLD_Z_NEG_AXIS);
    _fixedYawAxis.set(WORLD_Y_AXIS);
    _accumPitchDegrees = 0.0f;
    _orientation.identity();
    _viewMatrix.identity();
    _reflectedViewMatrix.identity();
    _yawFixed = false;
}

void Camera::saveCamera(){
    _savedVectors[0]  = _eye;
    _savedVectors[1]  = _fixedYawAxis;
    _savedFloats[0]   = _camIOD;
    _savedFloats[1]   = _moveSpeedFactor;
    _savedFloats[2]   = _turnSpeedFactor;
    _savedFloats[3]   = _mouseSensitivity;
    _savedOrientation = _orientation;
    _savedYawState    = _yawFixed;
    _saved = true;
}

void Camera::restoreCamera() {
    if(_saved) {
        _eye              = _savedVectors[0];
        _fixedYawAxis     = _savedVectors[1];
        _camIOD           = _savedFloats[0];
        _moveSpeedFactor  = _savedFloats[1];
        _turnSpeedFactor  = _savedFloats[2];
        _mouseSensitivity = _savedFloats[3];
        _orientation      = _savedOrientation;
        _yawFixed         = _savedYawState;
        updateViewMatrix();
        _saved = false;
    }
}

void Camera::update(const U64 deltaTime) {
    _cameraMoveSpeed = _moveSpeedFactor;
    _cameraTurnSpeed = _turnSpeedFactor;
#if USE_FIXED_TIMESTEP
    _cameraMoveSpeed *= Config::TICK_DIVISOR;
    _cameraTurnSpeed *= Config::TICK_DIVISOR;
#else
    _cameraMoveSpeed *= FRAME_SPEED_FACTOR;
    _cameraTurnSpeed *= FRAME_SPEED_FACTOR;
#endif
}

void Camera::setGlobalRotation(F32 yaw, F32 pitch, F32 roll) {
    Quaternion<F32> pitchRot(WORLD_X_AXIS, -pitch);
    Quaternion<F32> yawRot(WORLD_Y_AXIS, -yaw);
    if(!IS_ZERO(roll)){
        Quaternion<F32> rollRot;
        rollRot.fromAxisAngle(WORLD_Z_AXIS, -roll);
        setRotation(yawRot * pitchRot * rollRot);
    }else{
        setRotation(yawRot * pitchRot);
    }
}

void Camera::rotate(const Quaternion<F32>& q) {
    if(_type == FIRST_PERSON){
        vec3<F32> euler;
        q.getEuler(&euler);
        rotate(euler.yaw, euler.pitch,euler.roll);
    }else{
        Quaternion<F32> rot = q;
        rot.normalize();
        _orientation = rot * _orientation;
    }

     _viewMatrixDirty = true;
}

void Camera::rotate(F32 yaw, F32 pitch, F32 roll) {
    yaw   =   -yaw * _cameraTurnSpeed;
    pitch = -pitch * _cameraTurnSpeed;
    roll  =  -roll * _cameraTurnSpeed;

    if(_type == FIRST_PERSON){
        _accumPitchDegrees += pitch;

        if (_accumPitchDegrees > 90.0f){
            pitch = 90.0f - (_accumPitchDegrees - pitch);
            _accumPitchDegrees = 90.0f;
        }

        if (_accumPitchDegrees < -90.0f){
            pitch = -90.0f - (_accumPitchDegrees - pitch);
            _accumPitchDegrees = -90.0f;
        }

        Quaternion<F32> rot;

        // Rotate camera about the world y axis.
        // Note the order the quaternions are multiplied. That is important!
        if (yaw != 0.0f){
            rot.fromAxisAngle(WORLD_Y_AXIS, yaw);
            _orientation = rot * _orientation;
        }

        // Rotate camera about its local x axis.
        // Note the order the quaternions are multiplied. That is important!
        if (pitch != 0.0f){
            rot.fromAxisAngle(WORLD_X_AXIS, pitch);
            _orientation = _orientation * rot;
        }
    }else{
        _orientation *= Quaternion<F32>(pitch,yaw,roll);
    }

     _viewMatrixDirty = true;
}

void Camera::move(F32 dx, F32 dy, F32 dz) {
    _eye += _xAxis * dx * _cameraMoveSpeed;
    _eye += WORLD_Y_AXIS * dy * _cameraMoveSpeed;

    if (_type == FIRST_PERSON){
        // Calculate the forward direction. Can't just use the camera's local
        // z axis as doing so will cause the camera to move more slowly as the
        // camera's view approaches 90 degrees straight up and down.
        vec3<F32> forward;
        forward.cross(WORLD_Y_AXIS, _xAxis);
        forward.normalize();
        _eye += forward * dz * _cameraMoveSpeed;
    }else{
        _eye += _viewDir * dz * _cameraMoveSpeed;
    }

    _viewMatrixDirty = true;
}

void Camera::lookAt(const vec3<F32>& eye, const vec3<F32>& target, const vec3<F32>& up) {
    _eye = eye;

    _zAxis = eye - target;
    _zAxis.normalize();
    _xAxis.cross(up, _zAxis);
    _xAxis.normalize();
    _yAxis.cross(_zAxis, _xAxis);
    _yAxis.normalize();

    _viewDir = -_zAxis;

    _viewMatrix.m[0][0] = _xAxis.x;
    _viewMatrix.m[1][0] = _xAxis.y;
    _viewMatrix.m[2][0] = _xAxis.z;
    _viewMatrix.m[3][0] = -_xAxis.dot(eye);

    _viewMatrix.m[0][1] = _yAxis.x;
    _viewMatrix.m[1][1] = _yAxis.y;
    _viewMatrix.m[2][1] = _yAxis.z;
    _viewMatrix.m[3][1] = -_yAxis.dot(eye);

    _viewMatrix.m[0][2] = _zAxis.x;
    _viewMatrix.m[1][2] = _zAxis.y;
    _viewMatrix.m[2][2] = _zAxis.z;
    _viewMatrix.m[3][2] = -_zAxis.dot(eye);

    // Extract the pitch angle from the view matrix.
    _accumPitchDegrees = DEGREES(asinf(_zAxis.y));

    _orientation.fromMatrix(_viewMatrix);

    _viewMatrixDirty = false;
}

void Camera::setAnaglyph(bool rightEye){
    GFX_DEVICE.setAnaglyphFrustum(_camIOD, rightEye);
    if(rightEye) _eye.x += _camIOD/2;
    else         _eye.x -= _camIOD/2;

    _viewMatrixDirty = true;
}

///Tell the rendering API to set up our desired PoV
void Camera::renderLookAt(bool invertX) {
    updateViewMatrix();

    if(invertX) _viewMatrix.scale(vec3<F32>(-1,1,1));
    //Tell the Rendering API to draw from our desired PoV
    GFX_DEVICE.lookAt(_viewMatrix, _viewDir);
    //Extract the frustum associated with our current PoV
    Frustum::getInstance().Extract(_eye);
    //Inform all listeners of a new event
    updateListeners();
}

void Camera::renderLookAtReflected(const Plane<F32>& reflectionPlane, bool invertX){
    updateViewMatrix();

    _reflectedViewMatrix.reflect(reflectionPlane);
    if(invertX) _reflectedViewMatrix.scale(vec3<F32>(-1,1,1));
    GFX_DEVICE.lookAt(_reflectedViewMatrix * _viewMatrix, _reflectedViewMatrix * _viewDir);
    //Extract the frustum associated with our current PoV
    Frustum::getInstance().Extract(_reflectedViewMatrix * _eye);
    //Inform all listeners of a new event
    updateListeners();
}

void Camera::updateViewMatrix(){
    if(!_viewMatrixDirty)
        return;
        
    _orientation.normalize();

    // Reconstruct the view matrix.
    _viewMatrix = _orientation.getMatrix();

    _xAxis.set(_viewMatrix.m[0][0], _viewMatrix.m[1][0], _viewMatrix.m[2][0]);
    _yAxis.set(_viewMatrix.m[0][1], _viewMatrix.m[1][1], _viewMatrix.m[2][1]);
    _zAxis.set(_viewMatrix.m[0][2], _viewMatrix.m[1][2], _viewMatrix.m[2][2]);
    _viewDir = -_zAxis;
    _target = _viewDir + _eye;

    _viewMatrix.m[3][0] = -_xAxis.dot(_eye);
    _viewMatrix.m[3][1] = -_yAxis.dot(_eye);
    _viewMatrix.m[3][2] = -_zAxis.dot(_eye);
    _orientation.getEuler(&_euler,true);

    _viewMatrixDirty = false;
}

void Camera::updateListeners(){
    for_each(const DELEGATE_CBK& listener, _listeners){
        listener();
    }
}
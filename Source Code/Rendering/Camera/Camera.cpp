#include "Headers/Camera.h"

#include "Platform/Video/Headers/GFXDevice.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {

Camera::Camera(const stringImpl& name, const CameraType& type, const vec3<F32>& eye)
    : Resource(name),
      _isActive(false),
      _isOrthoCamera(false),
      _projectionDirty(true),
      _viewMatrixDirty(true),
      _viewMatrixLocked(false),
      _rotationLocked(false),
      _movementLocked(false),
      _frustumLocked(false),
      _frustumDirty(true),
      _mouseSensitivity(1.0f),
      _zoomSpeedFactor(35.0f),
      _moveSpeedFactor(35.0f),
      _turnSpeedFactor(35.0f),
      _cameraMoveSpeed(35.0f),
      _cameraZoomSpeed(35.0f),
      _cameraTurnSpeed(35.0f),
      _aspectRatio(1.77f),
      _verticalFoV(60.0f),
      _type(type)
{
    _eye.set(eye);
    _fixedYawAxis.set(WORLD_Y_AXIS);
    _accumPitchDegrees = 0.0f;
    _orientation.identity();
    _viewMatrix.identity();
    _yawFixed = false;
    _zPlanes.set(0.1f, 1.0f);
    _frustum = MemoryManager_NEW Frustum(*this);
}

Camera::~Camera()
{
    MemoryManager::DELETE(_frustum);
}

void Camera::fromCamera(const Camera& camera) {
    if (camera._isOrthoCamera) {
        setProjection(camera._orthoRect,
                      camera.getZPlanes());
    } else {
        setProjection(camera.getAspectRatio(),
                      camera.getVerticalFoV(),
                      camera.getZPlanes());
    }

    lookAt(camera.getEye(), camera.getTarget(), camera.getUpDir());
    setMoveSpeedFactor(camera.getMoveSpeedFactor());
    setTurnSpeedFactor(camera.getTurnSpeedFactor());
    setZoomSpeedFactor(camera.getZoomSpeedFactor());
    setFixedYawAxis(camera._yawFixed, camera._fixedYawAxis);
    lockView(camera._viewMatrixLocked);
    lockFrustum(camera._frustumLocked);

    _accumPitchDegrees = camera._accumPitchDegrees;
}

void Camera::update(const U64 deltaTime) {
    F32 timeFactor = Time::MicrosecondsToSeconds<F32>(deltaTime);
    _cameraMoveSpeed = _moveSpeedFactor * timeFactor;
    _cameraTurnSpeed = _turnSpeedFactor * timeFactor;
    _cameraZoomSpeed = _zoomSpeedFactor * timeFactor;
}

void Camera::updateProjection(bool updateGPU) {
    assert(_isActive);

    if (updateGPU) {
        GFXDevice& gfx = GFX_DEVICE;

        F32* projectionMatrixData = 
            _isOrthoCamera ? gfx.setProjection(_orthoRect,
                                               _zPlanes)

                           : gfx.setProjection(_verticalFoV,
                                               _aspectRatio,
                                               _zPlanes);
        if (_projectionDirty) {
            _projectionMatrix.set(projectionMatrixData);
            _frustumDirty = true;
            _projectionDirty = false;
        }

    } else {
        if (_projectionDirty) {
            if (_isOrthoCamera) {
                _projectionMatrix.ortho(_orthoRect.x,
                                        _orthoRect.y,
                                        _orthoRect.z,
                                        _orthoRect.w,
                                        _zPlanes.x,
                                        _zPlanes.y);
            } else {
                _projectionMatrix.perspective(Angle::DegreesToRadians(_verticalFoV),
                                              _aspectRatio,
                                              _zPlanes.x,
                                              _zPlanes.y);
            }
            _frustumDirty = true;
            _projectionDirty = false;
        }
    }
}

void Camera::onActivate() {
    _isActive = true;
    updateProjection(false);
}

void Camera::onDeactivate() {
    _isActive = false;
}

void Camera::setGlobalRotation(F32 yaw, F32 pitch, F32 roll) {
    if (_rotationLocked) {
        return;
    }

    Quaternion<F32> pitchRot(WORLD_X_AXIS, -pitch);
    Quaternion<F32> yawRot(WORLD_Y_AXIS, -yaw);

    if (!IS_ZERO(roll)) {
        Quaternion<F32> tempOrientation(WORLD_Z_AXIS, -roll);
        setRotation(yawRot * pitchRot * tempOrientation);
    } else {
        setRotation(yawRot * pitchRot);
    }
}

void Camera::rotate(const Quaternion<F32>& q) {
    if (_rotationLocked) {
        return;
    }

    if (_type == CameraType::FIRST_PERSON) {
        vec3<F32> euler;
        q.getEuler(euler);
        rotate(euler.yaw, euler.pitch, euler.roll);
    } else {
        Quaternion<F32> tempOrientation(q);
        tempOrientation.normalize();
        _orientation = tempOrientation * _orientation;
    }

    _viewMatrixDirty = true;
}

void Camera::rotate(F32 yaw, F32 pitch, F32 roll) {
    if (_rotationLocked) {
        return;
    }

    yaw = -yaw * _cameraTurnSpeed;
    pitch = -pitch * _cameraTurnSpeed;
    roll = -roll * _cameraTurnSpeed;

    Quaternion<F32> tempOrientation;
    if (_type == CameraType::FIRST_PERSON) {
        _accumPitchDegrees += pitch;

        if (_accumPitchDegrees > 90.0f) {
            pitch = 90.0f - (_accumPitchDegrees - pitch);
            _accumPitchDegrees = 90.0f;
        }

        if (_accumPitchDegrees < -90.0f) {
            pitch = -90.0f - (_accumPitchDegrees - pitch);
            _accumPitchDegrees = -90.0f;
        }

        // Rotate camera about the world y axis.
        // Note the order the quaternions are multiplied. That is important!
        if (!IS_ZERO(yaw)) {
            tempOrientation.fromAxisAngle(WORLD_Y_AXIS, yaw);
            _orientation = tempOrientation * _orientation;
        }

        // Rotate camera about its local x axis.
        // Note the order the quaternions are multiplied. That is important!
        if (!IS_ZERO(pitch)) {
            tempOrientation.fromAxisAngle(WORLD_X_AXIS, pitch);
            _orientation = _orientation * tempOrientation;
        }
    } else {
        tempOrientation.fromEuler(pitch, yaw, roll);
        _orientation *= tempOrientation;
    }

    _viewMatrixDirty = true;
}

void Camera::move(F32 dx, F32 dy, F32 dz) {
    if (_movementLocked) {
        return;
    }
    dx *= _cameraMoveSpeed;
    dy *= _cameraMoveSpeed;
    dz *= _cameraMoveSpeed;

    _eye += getRightDir() * dx;
    _eye += WORLD_Y_AXIS * dy;

    if (_type == CameraType::FIRST_PERSON) {
        // Calculate the forward direction. Can't just use the camera's local
        // z axis as doing so will cause the camera to move more slowly as the
        // camera's view approaches 90 degrees straight up and down.
        vec3<F32> forward;
        forward.cross(WORLD_Y_AXIS, getRightDir());
        forward.normalize();
        _eye += forward * dz;
    } else {
        _eye += getForwardDir() * dz;
    }

    _viewMatrixDirty = true;
}

const mat4<F32>& Camera::lookAt(const vec3<F32>& eye,
                                const vec3<F32>& target,
                                const vec3<F32>& up) {
    vec3<F32> xAxis, yAxis, zAxis;

    _eye.set(eye);
    _target.set(target);
    zAxis.set(eye - target);

    zAxis.normalize();
    xAxis.cross(up, zAxis);
    xAxis.normalize();
    yAxis.cross(zAxis, xAxis);
    yAxis.normalize();
    
    _viewMatrix.m[0][0] = xAxis.x;
    _viewMatrix.m[1][0] = xAxis.y;
    _viewMatrix.m[2][0] = xAxis.z;
    _viewMatrix.m[3][0] = xAxis.dot(eye);

    _viewMatrix.m[0][1] = yAxis.x;
    _viewMatrix.m[1][1] = yAxis.y;
    _viewMatrix.m[2][1] = yAxis.z;
    _viewMatrix.m[3][1] = yAxis.dot(eye);

    _viewMatrix.m[0][2] = zAxis.x;
    _viewMatrix.m[1][2] = zAxis.y;
    _viewMatrix.m[2][2] = zAxis.z;
    _viewMatrix.m[3][2] = zAxis.dot(eye);

    // Extract the pitch angle from the view matrix.
    _accumPitchDegrees = Angle::RadiansToDegrees(asinf(zAxis.y));

    _orientation.fromMatrix(_viewMatrix);

    _viewMatrixDirty = false;
    _frustumDirty = true;

    return _viewMatrix;
}

/// Tell the rendering API to set up our desired PoV
void Camera::renderLookAt(mat4<F32>& viewMatrixOut, vec3<F32>& eyeVecOut) {
    updateViewMatrix();
    updateProjection();
    updateFrustum();
    // Inform all listeners of a new event
    updateListeners();

    viewMatrixOut = _viewMatrix;
    eyeVecOut = _eye;
}

void Camera::reflect(const Plane<F32>& reflectionPlane) {
    mat4<F32> reflectedMatrix(getViewMatrix());
    reflectedMatrix.reflect(reflectionPlane);
    lookAt(reflectedMatrix * getEye(), getTarget(), reflectedMatrix * getUpDir());
}

void Camera::setProjection(F32 aspectRatio, F32 verticalFoV,
                           const vec2<F32>& zPlanes, bool updateOnSet) {
    _zPlanes = zPlanes;
    _aspectRatio = aspectRatio;
    _verticalFoV = verticalFoV;
    _isOrthoCamera = false;
    _projectionDirty = true;

    if (updateOnSet) {
        _isActive = true;
        updateProjection();
    }
}

void Camera::setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes, bool updateOnSet) {
    _zPlanes = zPlanes;
    _orthoRect = rect;
    _isOrthoCamera = true;
    _projectionDirty = true;

    if (updateOnSet) {
        _isActive = true;
        updateProjection();
    }
}

bool Camera::updateViewMatrix() {
    if (!_viewMatrixDirty || _viewMatrixLocked) {
        return false;
    }

    vec3<F32> xAxis, yAxis, zAxis;

    _orientation.normalize();

    // Reconstruct the view matrix.
    _orientation.getMatrix(_viewMatrix);

    xAxis.set(_viewMatrix.m[0][0], _viewMatrix.m[1][0], _viewMatrix.m[2][0]);
    yAxis.set(_viewMatrix.m[0][1], _viewMatrix.m[1][1], _viewMatrix.m[2][1]);
    zAxis.set(_viewMatrix.m[0][2], _viewMatrix.m[1][2], _viewMatrix.m[2][2]);

    _target = -zAxis + _eye;

    _viewMatrix.m[3][0] = -xAxis.dot(_eye);
    _viewMatrix.m[3][1] = -yAxis.dot(_eye);
    _viewMatrix.m[3][2] = -zAxis.dot(_eye);
    _orientation.getEuler(_euler, true);

    _viewMatrixDirty = false;
    _frustumDirty = true;

    return true;
}

void Camera::updateListeners() {
    assert(_isActive);

    for (const DELEGATE_CBK_PARAM<Camera&>& listener : _listeners) {
        listener(*this);
    }
}

bool Camera::updateFrustum() {
    assert(_isActive);

    if (_frustumLocked) {
        return true;
    }

    if (!_frustumDirty) {
        return false;
    }

    _frustum->Extract(_viewMatrix, _projectionMatrix);

    _frustumDirty = false;

    return true;
}

vec3<F32> Camera::unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ) const {
    const vec4<I32>& viewport = GFX_DEVICE.getCurrentViewport();

    return unProject(winCoordsX, winCoordsY, winCoordsZ, viewport);
}

vec3<F32> Camera::unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ,
                            const vec4<I32>& viewport) const {
    vec4<F32> temp(winCoordsX, winCoordsY, winCoordsZ, 1.0f);
    temp.x = (temp.x - F32(viewport[0])) / F32(viewport[2]);
    temp.y = (temp.y - F32(viewport[1])) / F32(viewport[3]);

    temp = _frustum->_viewProjectionMatrixCache.getInverse() * (2.0f * temp - 1.0f);
    temp /= temp.w;

    return temp.xyz();
}
};
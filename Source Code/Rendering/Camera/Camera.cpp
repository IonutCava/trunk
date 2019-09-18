#include "stdafx.h"

#include "Headers/Camera.h"

namespace Divide {

Camera::Camera(const stringImpl& name, const CameraType& type, const vec3<F32>& eye)
    : Resource(ResourceType::DEFAULT, name),
      _isOrthoCamera(false),
      _projectionDirty(true),
      _viewMatrixDirty(false),
      _rotationLocked(false),
      _movementLocked(false),
      _frustumLocked(false),
      _frustumDirty(true),
      _reflectionActive(false),
      _mouseSensitivity(1.0f),
      _zoomSpeedFactor(35.0f),
      _moveSpeedFactor(35.0f),
      _turnSpeedFactor(35.0f),
      _cameraMoveSpeed(35.0f),
      _cameraZoomSpeed(35.0f),
      _cameraTurnSpeed(35.0f),
      _type(type),
      _updateCameraId(0)
{
    _yawFixed = false;
    _fixedYawAxis.set(WORLD_Y_AXIS);
    _accumPitchDegrees = 0.0f;
    _data._eye.set(eye);
    _data._FoV = 60.0f;
    _data._aspectRatio = 1.77f;
    _data._viewMatrix.identity();
    _data._projectionMatrix.identity();
    _data._zPlanes.set(0.1f, 1.0f);
    _data._orientation.identity();
}

Camera::~Camera()
{
}

const CameraSnapshot& Camera::snapshot() {
    updateLookAt();

    return _data;
}

void Camera::fromCamera(Camera& camera) {
    camera.updateLookAt();
    fromCamera(static_cast<const Camera&>(camera));
}

void Camera::fromCamera(const Camera& camera) {
    setMoveSpeedFactor(camera.getMoveSpeedFactor());
    setTurnSpeedFactor(camera.getTurnSpeedFactor());
    setZoomSpeedFactor(camera.getZoomSpeedFactor());
    setFixedYawAxis(camera._yawFixed, camera._fixedYawAxis);
    lockMovement(camera._movementLocked);
    lockRotation(camera._rotationLocked);
    _mouseSensitivity = camera._mouseSensitivity;
    _cameraMoveSpeed = camera._cameraMoveSpeed;
    _cameraTurnSpeed = camera._cameraTurnSpeed;
    _cameraZoomSpeed = camera._cameraZoomSpeed;
    _reflectionPlane = camera._reflectionPlane;
    _reflectionActive = camera._reflectionActive;

    lockFrustum(camera._frustumLocked);
    _accumPitchDegrees = camera._accumPitchDegrees;

    if (camera._isOrthoCamera) {
        setAspectRatio(camera.getAspectRatio());
        setVerticalFoV(camera.getVerticalFoV());

        setProjection(camera._orthoRect,
                      camera.getZPlanes());
    } else {
        _orthoRect.set(camera._orthoRect);

        setProjection(camera.getAspectRatio(),
                      camera.getVerticalFoV(),
                      camera.getZPlanes());
    }

    setEye(camera.getEye());
    setRotation(camera._data._orientation);
    updateLookAt();
}

void Camera::fromSnapshot(const CameraSnapshot& snapshot) {
    setEye(snapshot._eye);
    setRotation(snapshot._orientation);
    setAspectRatio(snapshot._aspectRatio);
    if (_isOrthoCamera) {
        setProjection(_orthoRect, snapshot._zPlanes);
    } else {
        setProjection(snapshot._aspectRatio, snapshot._FoV, snapshot._zPlanes);
    }
    updateLookAt();
}

void Camera::updateInternal(const U64 deltaTimeUS) {
    F32 timeFactor = Time::MicrosecondsToSeconds<F32>(deltaTimeUS);
    _cameraMoveSpeed = _moveSpeedFactor * timeFactor;
    _cameraTurnSpeed = _turnSpeedFactor * timeFactor;
    _cameraZoomSpeed = _zoomSpeedFactor * timeFactor;
}

void Camera::setGlobalRotation(F32 yaw, F32 pitch, F32 roll) {
    if (_rotationLocked) {
        return;
    }

    Quaternion<F32> pitchRot(WORLD_X_AXIS, -pitch);
    Quaternion<F32> yawRot(WORLD_Y_AXIS, -yaw);

    if (!IS_ZERO(roll)) {
        setRotation(yawRot * pitchRot * Quaternion<F32>(WORLD_Z_AXIS, -roll));
    } else {
        setRotation(yawRot * pitchRot);
    }
}

void Camera::rotate(const Quaternion<F32>& q) {
    if (_rotationLocked) {
        return;
    }

    if (_type == CameraType::FIRST_PERSON) {
        vec3<Angle::DEGREES<F32>> euler;
        q.getEuler(euler);
        euler = Angle::to_DEGREES(euler);
        rotate(euler.yaw, euler.pitch, euler.roll);
    } else {
        _data._orientation = q * _data._orientation;
        _data._orientation.normalize();
    }

    _viewMatrixDirty = true;
}

void Camera::rotate(Angle::DEGREES<F32> yaw, Angle::DEGREES<F32> pitch, Angle::DEGREES<F32> roll) {
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
            _data._orientation = tempOrientation * _data._orientation;
        }

        // Rotate camera about its local x axis.
        // Note the order the quaternions are multiplied. That is important!
        if (!IS_ZERO(pitch)) {
            tempOrientation.fromAxisAngle(WORLD_X_AXIS, pitch);
            _data._orientation = _data._orientation * tempOrientation;
        }
    } else {
        tempOrientation.fromEuler(pitch, yaw, roll);
        _data._orientation *= tempOrientation;
    }

    _viewMatrixDirty = true;
}

void Camera::rotateYaw(Angle::DEGREES<F32> angle) {
    rotate(Quaternion<F32>(_yawFixed ? _fixedYawAxis : _data._orientation * WORLD_Y_AXIS, -angle * _cameraTurnSpeed));
}

void Camera::rotateRoll(Angle::DEGREES<F32> angle) {
    rotate(Quaternion<F32>(_data._orientation * WORLD_Z_AXIS, -angle * _cameraTurnSpeed));
}

void Camera::rotatePitch(Angle::DEGREES<F32> angle) {
    rotate(Quaternion<F32>(_data._orientation * WORLD_X_AXIS, -angle * _cameraTurnSpeed));
}

void Camera::move(F32 dx, F32 dy, F32 dz) {
    if (_movementLocked || IS_ZERO(dx + dy + dz)) {
        return;
    }
    
    dx *= _cameraMoveSpeed;
    dy *= _cameraMoveSpeed;
    dz *= _cameraMoveSpeed;

    _data._eye += getRightDir() * dx;
    _data._eye += WORLD_Y_AXIS * dy;

    if (_type == CameraType::FIRST_PERSON) {
        // Calculate the forward direction. Can't just use the camera's local
        // z axis as doing so will cause the camera to move more slowly as the
        // camera's view approaches 90 degrees straight up and down.
        const vec3<F32> forward = Normalized(Cross(WORLD_Y_AXIS, getRightDir()));
        _data._eye += forward * dz;
    } else {
        _data._eye += getForwardDir() * dz;
    }

    _viewMatrixDirty = true;
}

bool Camera::moveRelative(const vec3<I32>& relMovement) {
    if (relMovement.lengthSquared() > 0) {
        move(to_F32(relMovement.y), to_F32(relMovement.z), to_F32(relMovement.x));
        return true;
    }

    return false;
}

bool Camera::rotateRelative(const vec3<I32>& relRotation) {
    if (relRotation.lengthSquared() > 0) {
        rotate(Quaternion<F32>(_yawFixed ? _fixedYawAxis : _data._orientation * WORLD_Y_AXIS, -relRotation.yaw * _cameraTurnSpeed) *
               Quaternion<F32>(_data._orientation * WORLD_X_AXIS, -relRotation.pitch * _cameraTurnSpeed) *
               Quaternion<F32>(_data._orientation * WORLD_Z_AXIS, -relRotation.roll * _cameraTurnSpeed));
        return true;
    }
    return false;
}

bool Camera::zoom(I32 zoomFactor) {
    return zoomFactor != 0;
}

vec3<F32> ExtractCameraPos2(const mat4<F32>& a_modelView)
{
    // Get the 3 basis vector planes at the camera origin and transform them into model space.
    //  
    // NOTE: Planes have to be transformed by the inverse transpose of a matrix
    //       Nice reference here: http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
    //
    //       So for a transform to model space we need to do:
    //            inverse(transpose(inverse(MV)))
    //       This equals : transpose(MV) - see Lemma 5 in http://mathrefresher.blogspot.com.au/2007/06/transpose-of-matrix.html
    //
    // As each plane is simply (1,0,0,0), (0,1,0,0), (0,0,1,0) we can pull the data directly from the transpose matrix.
    //  
    mat4<F32> modelViewT(a_modelView.getTranspose());

    // Get plane normals 
    vec4<F32> n1(modelViewT.getRow(0));
    vec4<F32> n2(modelViewT.getRow(1));
    vec4<F32> n3(modelViewT.getRow(2));

    // Get plane distances
    F32 d1(n1.w);
    F32 d2(n2.w);
    F32 d3(n3.w);

    // Get the intersection of these 3 planes 
    // (uisng math from RealTime Collision Detection by Christer Ericson)
    vec3<F32> n2n3 = Cross(n2.xyz(), n3.xyz());
    float denom = Dot(n1.xyz(), n2n3);

    vec3<F32> top = (n2n3 * d1) + Cross(n1.xyz(), (d3*n2.xyz()) - (d2*n3.xyz()));
    return top / -denom;
}
const mat4<F32>& Camera::lookAt(const mat4<F32>& viewMatrix) {
    _data._eye.set(ExtractCameraPos2(viewMatrix));
    _data._orientation.fromMatrix(viewMatrix);
    _viewMatrixDirty = true;
    _frustumDirty = true;
    updateViewMatrix();

    return getViewMatrix();
}

const mat4<F32>& Camera::lookAt(const vec3<F32>& eye,
                                const vec3<F32>& target,
                                const vec3<F32>& up) {
    _data._eye.set(eye);
    _data._orientation.fromMatrix(mat4<F32>(eye, target, up));
    _viewMatrixDirty = true;
    _frustumDirty = true;

    updateViewMatrix();

    return getViewMatrix();
}

/// Tell the rendering API to set up our desired PoV
bool Camera::updateLookAt() {
    bool cameraUpdated =  updateViewMatrix();
    cameraUpdated = updateProjection() || cameraUpdated;
    cameraUpdated = updateFrustum() || cameraUpdated;
    
    if (cameraUpdated) {
        for (ListenerMap::value_type it : _updateCameraListeners) {
            it.second(*this);
        }
    }

    return cameraUpdated;
}

bool Camera::removeUpdateListener(U32 id) {
    ListenerMap::const_iterator it = _updateCameraListeners.find(id);
    if (it != std::cend(_updateCameraListeners)) {
        _updateCameraListeners.erase(it);
        return true;
    }

    return false;
}

U32 Camera::addUpdateListener(const DELEGATE_CBK<void, const Camera&>& f) {
    hashAlg::insert(_updateCameraListeners, ++_updateCameraId, f);
    return _updateCameraId;
}

void Camera::setReflection(const Plane<F32>& reflectionPlane) {
    _reflectionPlane = reflectionPlane;
    _reflectionActive = true;
    _viewMatrixDirty = true;
}

void Camera::clearReflection() {
    _reflectionActive = false;
    _viewMatrixDirty = true;
}

bool Camera::updateProjection() {
    if (_projectionDirty) {
        if (_isOrthoCamera) {
            _data._projectionMatrix.ortho(_orthoRect.left,
                                          _orthoRect.right,
                                          _orthoRect.bottom,
                                          _orthoRect.top,
                                          _data._zPlanes.x,
                                          _data._zPlanes.y);
        } else {
            _data._projectionMatrix.perspective(_data._FoV,
                                                _data._aspectRatio,
                                                _data._zPlanes.x,
                                                _data._zPlanes.y);
        }

        _frustumDirty = true;
        _projectionDirty = false;
        return true;
    }

    return false;
}

const mat4<F32>& Camera::setProjection(F32 aspectRatio, F32 verticalFoV, const vec2<F32>& zPlanes) {
    setAspectRatio(_data._aspectRatio);
    setVerticalFoV(verticalFoV);

    _data._zPlanes = zPlanes;
    _isOrthoCamera = false;
    _projectionDirty = true;
    updateProjection();

    return getProjectionMatrix();
}

const mat4<F32>& Camera::setProjection(const vec4<F32>& rect, const vec2<F32>& zPlanes) {
    _data._zPlanes = zPlanes;
    _orthoRect = rect;
    _isOrthoCamera = true;
    _projectionDirty = true;
    updateProjection();

    return getProjectionMatrix();
}

const mat4<F32>& Camera::setProjection(const mat4<F32>& projection, const vec2<F32>& zPlanes, bool isOrtho) {
    _data._projectionMatrix.set(projection);
    _data._zPlanes = zPlanes;
    _projectionDirty = false;
    _frustumDirty = true;
    _isOrthoCamera = isOrtho;

    return _data._projectionMatrix;
}

void Camera::setAspectRatio(F32 ratio) {
    _data._aspectRatio = ratio;
    _projectionDirty = true;
}

void Camera::setVerticalFoV(Angle::DEGREES<F32> verticalFoV) {
    _data._FoV = verticalFoV;
    _projectionDirty = true;
}

void Camera::setHorizontalFoV(Angle::DEGREES<F32> horizontalFoV) {
    _data._FoV = Angle::to_DEGREES(2.0f * std::atan(tan(Angle::to_RADIANS(horizontalFoV) * 0.5f) / _data._aspectRatio));
    _projectionDirty = true;
}

bool Camera::updateViewMatrix() {
    if (!_viewMatrixDirty) {
        return false;
    }

    _data._orientation.normalize();

    // Reconstruct the view matrix.
    vec3<F32> xAxis = _data._orientation.xAxis();
    vec3<F32> yAxis = _data._orientation.yAxis();
    vec3<F32> zAxis = _data._orientation.zAxis();

    vec3<F32> target = -zAxis + _data._eye;

    _data._viewMatrix.set(GetMatrix(_data._orientation));
    _data._viewMatrix.setRow(3, -xAxis.dot(_data._eye), -yAxis.dot(_data._eye), -zAxis.dot(_data._eye), 1.0f);
    _data._orientation.getEuler(_euler);
    _euler = Angle::to_DEGREES(_euler);
    // Extract the pitch angle from the view matrix.
    _accumPitchDegrees = Angle::to_DEGREES(std::asinf(getForwardDir().y));

    if (_reflectionActive) {
        _data._viewMatrix.reflect(_reflectionPlane);
        _data._eye.set(mat4<F32>(_reflectionPlane).transformNonHomogeneous(_data._eye));
    }

    _viewMatrixDirty = false;
    _frustumDirty = true;

    return true;
}

bool Camera::updateFrustum() {
    if (_frustumLocked) {
        return true;
    }

    if (!_frustumDirty) {
        return false;
    }

    _frustum.Extract(getViewMatrix(), getProjectionMatrix());

    _frustumDirty = false;

    return true;
}

vec3<F32> Camera::unProject(F32 winCoordsX, F32 winCoordsY, F32 winCoordsZ, const Rect<I32>& viewport) const {
    Rect<F32> temp(winCoordsX, winCoordsY, winCoordsZ, 1.0f);
    temp.x = (temp.x - F32(viewport[0])) / F32(viewport[2]);
    temp.y = (temp.y - F32(viewport[1])) / F32(viewport[3]);

    temp = (getViewMatrix() * getProjectionMatrix()).getInverse() * (2.0f * temp - 1.0f);
    temp /= temp.w;

    return temp.xyz();
}

};

#include "Headers/OrbitCamera.h"

#include "Core/Headers/Application.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

OrbitCamera::OrbitCamera(const stringImpl& name, const CameraType& type, const vec3<F32>& eye)
    : Camera(name, type, eye),
      _currentRotationX(0.0),
      _currentRotationY(0.0),
      _maxRadius(10.0f),
      _minRadius(0.1f),
      _curRadius(8.0f),
      _rotationDirty(true)
{
    setMouseSensitivity(0.5f);
}

void OrbitCamera::fromCamera(Camera& camera) {
    Camera::fromCamera(camera);
    if (camera.getType() != CameraType::THIRD_PERSON && camera.getType() != CameraType::ORBIT) {
        return;
    }

    OrbitCamera& orbitCam = static_cast<OrbitCamera&>(camera);

    _targetNode = orbitCam._targetNode;
    _maxRadius = orbitCam._maxRadius;
    _minRadius = orbitCam._minRadius;
    _curRadius = orbitCam._curRadius;
    _currentRotationX = orbitCam._currentRotationX;
    _currentRotationY = orbitCam._currentRotationY;
    _offsetDir.set(orbitCam._offsetDir);
    _cameraRotation.set(orbitCam._cameraRotation);
    _newEye.set(orbitCam._newEye);

    _rotationDirty = true;

}

void OrbitCamera::setTarget(SceneGraphNode_wptr sgn, const vec3<F32>& offsetDirection) {
    _targetNode = sgn;
    _offsetDir = offsetDirection;
    _offsetDir.normalize();
}

void OrbitCamera::setActiveInternal(bool state) {
    if (state) {
        //_cameraRotation.reset();
    }

    Camera::setActiveInternal(state);
}


bool OrbitCamera::updateViewMatrix() {
    setEye(_newEye);

    return Camera::updateViewMatrix();
}

void OrbitCamera::update(const U64 deltaTime) {
    Camera::update(deltaTime);

    SceneGraphNode_ptr sgn = _targetNode.lock();
    if (!sgn) {
        return;
    }

    PhysicsComponent* const trans =
        sgn->get<PhysicsComponent>();

    static vec3<F32> newTargetOrientation;

    if (/*trans->changedLastFrame() || */ _rotationDirty || true) {
        trans->getOrientation().getEuler(newTargetOrientation);
        newTargetOrientation.yaw = to_F32(M_PI) - newTargetOrientation.yaw;
        newTargetOrientation += _cameraRotation;
        Util::Normalize(newTargetOrientation, false);
        _rotationDirty = false;
    }

    _orientation.fromEuler(Angle::to_DEGREES(newTargetOrientation));
    _newEye = trans->getPosition() + _orientation * (_offsetDir * _curRadius);
    _viewMatrixDirty = true;
}

bool OrbitCamera::moveRelative(const vec3<I32>& relMovement) {
    I32 zoom = relMovement.z;
    if (zoom != 0) {
        curRadius(_curRadius += (zoom * _cameraZoomSpeed * -0.01f));
    }

    return Camera::moveRelative(relMovement);
}

void OrbitCamera::move(F32 dx, F32 dy, F32 dz) {}

void OrbitCamera::rotate(F32 yaw, F32 pitch, F32 roll) {}

};  // namespace Divide
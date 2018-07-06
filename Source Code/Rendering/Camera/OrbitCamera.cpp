#include "Headers/OrbitCamera.h"

#include "Core/Headers/Application.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

OrbitCamera::OrbitCamera(const CameraType& type, const vec3<F32>& eye)
    : Camera(type, eye),
      _currentRotationX(0.0),
      _currentRotationY(0.0),
      _maxRadius(10.0f),
      _minRadius(0.1f),
      _curRadius(8.0f),
      _rotationDirty(true)
{
    setMouseSensitivity(0.5f);
}

void OrbitCamera::setTarget(SceneGraphNode_wptr sgn, const vec3<F32>& offsetDirection) {
    _targetNode = sgn;
    _offsetDir = offsetDirection;
    _offsetDir.normalize();
}

void OrbitCamera::onActivate() {
    //_cameraRotation.reset();
    Camera::onActivate();
}

void OrbitCamera::onDeactivate() {}

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
        sgn->getComponent<PhysicsComponent>();

    static vec3<F32> newTargetOrientation;

    if (/*trans->changedLastFrame() || */ _rotationDirty || true) {
        trans->getOrientation().getEuler(&newTargetOrientation);
        newTargetOrientation.yaw = to_float(M_PI) - newTargetOrientation.yaw;
        newTargetOrientation += _cameraRotation;
        Util::Normalize(newTargetOrientation, false);
        _rotationDirty = false;
    }

    _orientation.fromEuler(newTargetOrientation, false);
    _newEye = trans->getPosition() + _orientation * (_offsetDir * _curRadius);
    _viewMatrixDirty = true;
}

bool OrbitCamera::mouseMoved(const Input::MouseEvent& arg) {
    I32 zoom = arg.state.Z.rel;
    if (zoom != 0) {
        curRadius(_curRadius += (zoom * _cameraZoomSpeed * -0.01f));
    }

    return Camera::mouseMoved(arg);
}

void OrbitCamera::move(F32 dx, F32 dy, F32 dz) {}

void OrbitCamera::rotate(F32 yaw, F32 pitch, F32 roll) {}

};  // namespace Divide
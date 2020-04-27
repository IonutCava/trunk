#include "stdafx.h"

#include "Headers/OrbitCamera.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "ECS/Components//Headers/TransformComponent.h"

namespace Divide {

OrbitCamera::OrbitCamera(const Str128& name, const CameraType& type, const vec3<F32>& eye)
    : FreeFlyCamera(name, type, eye)
{
    setMouseSensitivity(0.5f);
}

void OrbitCamera::fromCamera(const Camera& camera, bool flag) {
    if (camera.type() == Type() || flag) {
        const OrbitCamera& orbitCam = static_cast<const OrbitCamera&>(camera);
        _maxRadius = orbitCam._maxRadius;
        _minRadius = orbitCam._minRadius;
        _curRadius = orbitCam._curRadius;
        _currentRotationX = orbitCam._currentRotationX;
        _currentRotationY = orbitCam._currentRotationY;
        _rotationDirty = true;
        _offsetDir.set(orbitCam._offsetDir);
        _cameraRotation.set(orbitCam._cameraRotation);
        _newEye.set(orbitCam._newEye);
        _targetNode = orbitCam._targetNode;
        flag = true;
    }

    FreeFlyCamera::fromCamera(camera, flag);
}

void OrbitCamera::setTarget(SceneGraphNode* sgn, const vec3<F32>& offsetDirection) {
    _targetNode = sgn;
    _offsetDir = offsetDirection;
    _offsetDir.normalize();
}

bool OrbitCamera::updateViewMatrix() {
    setEye(_newEye);

    return FreeFlyCamera::updateViewMatrix();
}

void OrbitCamera::update(const U64 deltaTimeUS) {
    FreeFlyCamera::update(deltaTimeUS);

    if (!_targetNode) {
        return;
    }

    TransformComponent* const trans = _targetNode->get<TransformComponent>();

    static vec3<F32> newTargetOrientation;

    if (/*trans->changedLastFrame() || */ _rotationDirty || true) {
        trans->getOrientation().getEuler(newTargetOrientation);
        newTargetOrientation.yaw = to_F32(M_PI) - newTargetOrientation.yaw;
        newTargetOrientation += _cameraRotation;
        Util::Normalize(newTargetOrientation, false);
        _rotationDirty = false;
    }

    _data._orientation.fromEuler(Angle::to_DEGREES(newTargetOrientation));
    _newEye = trans->getPosition() + _data._orientation * (_offsetDir * _curRadius);
    _viewMatrixDirty = true;
}

bool OrbitCamera::zoom(I32 zoomFactor) noexcept {
    if (zoomFactor != 0) {
        curRadius(_curRadius += (zoomFactor * _speed.zoom * -0.01f));
    }

    return FreeFlyCamera::zoom(zoomFactor);
}

};  // namespace Divide
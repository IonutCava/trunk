#include "Headers/OrbitCamera.h"

#include "Core/Headers/Application.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

OrbitCamera::OrbitCamera(const CameraType& type, const vec3<F32>& eye) : Camera(type, eye),
                                                                         _currentRotationX(0.0),
                                                                         _currentRotationY(0.0),
                                                                         _maxRadius(10.0f),
                                                                         _minRadius(0.1f),
                                                                         _curRadius(8.0f),
                                                                         _targetNode(NULL),
                                                                         _rotationDirty(true)
{
    setMouseSensitivity(0.5f);
}


void OrbitCamera::setTarget(SceneGraphNode* const sgn, const vec3<F32>& offsetDirection) {
    assert(sgn != NULL);
    _targetNode = sgn;
    _offsetDir = offsetDirection;
    _offsetDir.normalize();
}

void OrbitCamera::onActivate() {
}

void OrbitCamera::onDeactivate() {
    _cameraRotation.reset();
}

void OrbitCamera::update(const U64 deltaTime) {
    Camera::update(deltaTime);

    Transform* trans = NULL;
    if(!_targetNode)
        return;
    else
        trans = _targetNode->getTransform();

    static vec3<F32> newTargetOrientation;
    static vec3<F32> newTargetPosition;

    if(/*trans->changedLastFrame() || */_rotationDirty || true){
        newTargetPosition = trans->getPosition();
        trans->getOrientation().getEuler(&newTargetOrientation);
        newTargetOrientation.yaw = M_PI - newTargetOrientation.yaw;
        newTargetOrientation += _cameraRotation;
        Util::normalize(newTargetOrientation, false);
        _rotationDirty = false;
    }

    _orientation.fromEuler(newTargetOrientation, false);
    setEye(newTargetPosition + _orientation * (_offsetDir * _curRadius));
}

bool OrbitCamera::onMouseMove(const OIS::MouseEvent& arg) {
    return true;
}

void OrbitCamera::move(F32 dx, F32 dy, F32 dz) {
}

void OrbitCamera::rotate(F32 yaw, F32 pitch, F32 roll) {
}
#include "stdafx.h"

#include "Headers/ThirdPersonCamera.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

ThirdPersonCamera::ThirdPersonCamera(const stringImpl& name, const vec3<F32>& eye)
    : OrbitCamera(name, CameraType::THIRD_PERSON, eye)
{
}

bool ThirdPersonCamera::moveRelative(const vec3<I32>& relMovement) {
    static vec2<F32> movePos;
    static const F32 rotationLimitRollLower = to_F32(M_PI) * 0.30f - Angle::to_RADIANS(1);
    static const F32 rotationLimitRollUpper = to_F32(M_PI) * 0.175f - Angle::to_RADIANS(1);
    static const F32 rotationLimitYaw = to_F32(M_PI) - Angle::to_RADIANS(1);

    movePos.set(relMovement.x, relMovement.y);

    if (IS_ZERO(movePos.x) && IS_ZERO(movePos.y)) {
        return OrbitCamera::moveRelative(relMovement);
    }

    movePos.set((movePos / _mouseSensitivity) * _turnSpeedFactor);

    if (!IS_ZERO(movePos.y)) {
        F32 targetRoll = _cameraRotation.roll - movePos.y;
        if (targetRoll > -rotationLimitRollLower &&
            targetRoll < rotationLimitRollUpper) {
            _cameraRotation.roll -= movePos.y;  // why roll? beats me.
            _rotationDirty = true;
        }
    }

    if (!IS_ZERO(movePos.x)) {
        F32 targetYaw = _cameraRotation.yaw - movePos.x;
        if (targetYaw > -rotationLimitYaw && targetYaw < rotationLimitYaw) {
            _cameraRotation.yaw -= movePos.x;
            _rotationDirty = true;
        }
    }

    if (_rotationDirty) {
        Util::Normalize(_cameraRotation, false, true, false, true);
    }

    return OrbitCamera::moveRelative(relMovement);
}
};
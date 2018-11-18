#include "stdafx.h"

#include "Headers/ThirdPersonCamera.h"
#include "Graphs/Headers/SceneGraphNode.h"

namespace Divide {

ThirdPersonCamera::ThirdPersonCamera(const stringImpl& name, const vec3<F32>& eye)
    : OrbitCamera(name, CameraType::THIRD_PERSON, eye)
{
}

bool ThirdPersonCamera::rotateRelative(const vec3<I32>& relRotation) {
    static vec3<F32> rotation;
    static const F32 rotationLimitRollLower = to_F32(M_PI) * 0.30f - Angle::to_RADIANS(1);
    static const F32 rotationLimitRollUpper = to_F32(M_PI) * 0.175f - Angle::to_RADIANS(1);
    static const F32 rotationLimitPitch = to_F32(M_PI) - Angle::to_RADIANS(1);

    if (relRotation.lengthSquared() != 0) {

        rotation.set(relRotation.pitch, relRotation.yaw, relRotation.roll);
        rotation.set((rotation / _mouseSensitivity) * _turnSpeedFactor);

        if (!IS_ZERO(rotation.yaw)) {
            F32 targetYaw = _cameraRotation.yaw - rotation.yaw;
            if (targetYaw > -rotationLimitRollLower &&
                targetYaw < rotationLimitRollUpper) {
                _cameraRotation.yaw -= rotation.yaw;
                _rotationDirty = true;
            }
        }

        if (!IS_ZERO(rotation.pitch)) {
            F32 targetPitch = _cameraRotation.yaw - rotation.pitch;
            if (targetPitch > -rotationLimitPitch && targetPitch < rotationLimitPitch) {
                _cameraRotation.pitch -= rotation.pitch;
                _rotationDirty = true;
            }
        }

        if (_rotationDirty) {
            Util::Normalize(_cameraRotation, false, true, false, true);
        }
    }

    return OrbitCamera::rotateRelative(relRotation);
}
};
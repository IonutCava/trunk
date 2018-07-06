#include "Headers/ThirdPersonCamera.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Input/Headers/InputInterface.h"

ThirdPersonCamera::ThirdPersonCamera(const vec3<F32>& eye) : OrbitCamera(THIRD_PERSON, eye)
{
}

void ThirdPersonCamera::onActivate() {
    Application::getInstance().snapCursorToCenter();
    OrbitCamera::onActivate();
}

bool ThirdPersonCamera::mouseMoved(const OIS::MouseEvent& arg) {
    static vec2<F32> mousePos;
    static const F32 rotationLimitRollLower = M_PI * 0.30f - RADIANS(1);
    static const F32 rotationLimitRollUpper = M_PI * 0.175f - RADIANS(1);
    static const F32 rotationLimitYaw = M_PI - RADIANS(1);

    mousePos.set(arg.state.X.rel, arg.state.Y.rel);

    Application::getInstance().snapCursorToCenter();

    if (IS_ZERO(mousePos.x) && IS_ZERO(mousePos.y)){
        return OrbitCamera::mouseMoved(arg);
    }

    mousePos.set((mousePos / _mouseSensitivity) * _turnSpeedFactor);

    if (!IS_ZERO(mousePos.y)) {
        F32 targetRoll = _cameraRotation.roll - mousePos.y;
        if(targetRoll > -rotationLimitRollLower && targetRoll < rotationLimitRollUpper){
            _cameraRotation.roll -= mousePos.y; //why roll? beats me.
            _rotationDirty = true;
        }
    }
    if (!IS_ZERO(mousePos.x)) {
        F32 targetYaw = _cameraRotation.yaw - mousePos.x;
        if(targetYaw > -rotationLimitYaw && targetYaw < rotationLimitYaw){
            _cameraRotation.yaw -= mousePos.x;
            _rotationDirty = true;
        }
    }
     
    if(_rotationDirty)
        Util::normalize(_cameraRotation, false,true, false, true);

    return OrbitCamera::mouseMoved(arg);
}
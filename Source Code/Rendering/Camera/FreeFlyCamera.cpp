#include "Headers/FreeFlyCamera.h"

namespace Divide {

FreeFlyCamera::FreeFlyCamera(const vec3<F32>& eye)
    : Camera(CameraType::FREE_FLY, eye)
{
}

void FreeFlyCamera::update(const U64 deltaTime) {
    Camera::update(deltaTime);
}

void FreeFlyCamera::move(F32 dx, F32 dy, F32 dz) {
    Camera::move(dx, dy, dz);

    _targetPosition = _eye + vec3<F32>(dx, dy, dz);
}

};
#include "stdafx.h"

#include "Headers/FreeFlyCamera.h"

namespace Divide {

FreeFlyCamera::FreeFlyCamera(const Str64& name, const vec3<F32>& eye)
    : Camera(name, CameraType::FREE_FLY, eye)
{
}

void FreeFlyCamera::fromCamera(Camera& camera) {
    Camera::fromCamera(camera);

    FreeFlyCamera& cam = static_cast<FreeFlyCamera&>(camera);
    _targetPosition.set(cam._targetPosition);
    _currentVelocity.set(cam._currentVelocity);
}

void FreeFlyCamera::update(const U64 deltaTimeUS) {
    Camera::update(deltaTimeUS);
}

void FreeFlyCamera::move(F32 dx, F32 dy, F32 dz) {
    Camera::move(dx, dy, dz);
}

};
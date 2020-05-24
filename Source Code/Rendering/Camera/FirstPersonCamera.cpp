#include "stdafx.h"

#include "Headers/FirstPersonCamera.h"

namespace Divide {

FirstPersonCamera::FirstPersonCamera(const Str256& name, const vec3<F32>& eye)
    : FreeFlyCamera(name, Type(), eye)
{
}

void FirstPersonCamera::fromCamera(const Camera& camera, bool flag) {
    FreeFlyCamera::fromCamera(camera, camera.type() == Type() || flag);
}

};
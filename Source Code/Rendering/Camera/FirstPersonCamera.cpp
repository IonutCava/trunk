#include "stdafx.h"

#include "Headers/FirstPersonCamera.h"

namespace Divide {

FirstPersonCamera::FirstPersonCamera(const stringImpl& name, const vec3<F32>& eye)
    : Camera(name, CameraType::FIRST_PERSON, eye)
{
}

};
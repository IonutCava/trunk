#include "stdafx.h"

#include "Headers/ScriptedCamera.h"

namespace Divide {

ScriptedCamera::ScriptedCamera(const Str128& name, const vec3<F32>& eye)
    : Camera(name, CameraType::SCRIPTED, eye)
{
}

};
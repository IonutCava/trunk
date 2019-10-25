#include "stdafx.h"

#include "Headers/ScriptedCamera.h"

namespace Divide {

ScriptedCamera::ScriptedCamera(const Str64& name, const vec3<F32>& eye)
    : Camera(name, CameraType::SCRIPTED, eye)
{
}

};
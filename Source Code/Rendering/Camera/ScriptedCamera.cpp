#include "Headers/ScriptedCamera.h"

namespace Divide {

ScriptedCamera::ScriptedCamera(const stringImpl& name, const vec3<F32>& eye)
    : Camera(name, CameraType::SCRIPTED, eye)
{
}

};
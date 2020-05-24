#include "stdafx.h"

#include "Headers/ScriptedCamera.h"

namespace Divide {

ScriptedCamera::ScriptedCamera(const Str256& name, const vec3<F32>& eye)
    : FreeFlyCamera(name, Type(), eye)
{
}

};
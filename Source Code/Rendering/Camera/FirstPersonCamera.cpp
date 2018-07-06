#include "Headers/FirstPersonCamera.h"

#include "Core/Headers/Application.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Platform/Input/Headers/InputInterface.h"

namespace Divide {

FirstPersonCamera::FirstPersonCamera(const vec3<F32>& eye) : Camera(FIRST_PERSON, eye)
{
}

};
#include "Headers/FirstPersonCamera.h"

#include "Core/Headers/Application.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"
#include "Hardware/Input/Headers/InputInterface.h"

FirstPersonCamera::FirstPersonCamera(const vec3<F32>& eye) : Camera(FIRST_PERSON, eye)
{
}
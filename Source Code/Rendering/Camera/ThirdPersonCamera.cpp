#include "Headers/ThirdPersonCamera.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

ThirdPersonCamera::ThirdPersonCamera(const vec3<F32>& eye) : OrbitCamera(THIRD_PERSON, eye)
{
}
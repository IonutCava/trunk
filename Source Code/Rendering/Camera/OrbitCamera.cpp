#include "Headers/OrbitCamera.h"
#include "Core/Math/Headers/Transform.h"
#include "Graphs/Headers/SceneGraphNode.h"

OrbitCamera::OrbitCamera(const CameraType& type, const vec3<F32>& eye) : Camera(type, eye)
{
}
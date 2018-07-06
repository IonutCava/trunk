#include "Headers/FreeFlyCamera.h"

namespace Divide {

FreeFlyCamera::FreeFlyCamera(const vec3<F32>& eye)
    : Camera(CameraType::FREE_FLY, eye) {}
};
#include "Headers/PointLight.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

PointLight::PointLight(F32 range) : Light(range, LightType::POINT) {
    _properties._position.set(0, 0, 0, 1.0f);
    // +x
    _direction[0].set(WORLD_X_AXIS);
    // -x
    _direction[1].set(WORLD_X_NEG_AXIS);
    // +z
    _direction[2].set(WORLD_Z_AXIS);
    // -z
    _direction[3].set(WORLD_Z_NEG_AXIS);
    // +y
    _direction[4].set(WORLD_Y_AXIS);
    // -y
    _direction[5].set(WORLD_Y_NEG_AXIS);
}
};
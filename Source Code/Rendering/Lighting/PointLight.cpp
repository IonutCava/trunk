#include "Headers/PointLight.h"
#include "Platform/Video/Headers/GFXDevice.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

PointLight::PointLight(F32 range) : Light(range, LightType::POINT) {
    _properties._position.set(0, 0, 0, 1.0f);
    // +x
    _direction[0].set(1.0f, 0.0f, 0.0f);
    // -x
    _direction[1].set(-1.0f, 0.0f, 0.0f);
    // +z
    _direction[2].set(0.0f, 0.0f, 1.0f);
    // -z
    _direction[3].set(0.0f, 0.0f, -1.0f);
    // +y
    _direction[4].set(0.0f, 1.0f, 0.0f);
    // -y
    _direction[5].set(0.0f, -1.0f, 0.0f);
}
};
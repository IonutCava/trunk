#include "Headers/PointLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

PointLight::PointLight(U8 slot, F32 range) : Light(slot,range,LIGHT_TYPE_POINT) {
	_properties._position = vec4<F32>(0,0,0,1.0f);
};
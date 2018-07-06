#include "Headers/PointLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"

#include "Graphs/Headers/SceneGraphNode.h"
#include "Core/Math/Headers/Transform.h"

namespace Divide {

PointLight::PointLight(F32 range) : Light(range, LIGHT_TYPE_POINT) 
{
	_properties._position = vec4<F32>(0,0,0,1.0f);
    // +x
    _direction[0] = vec3<F32>( 1.0f,  0.0f,  0.0f);
    // -x
    _direction[1] = vec3<F32>(-1.0f,  0.0f,  0.0f);
    // +z
    _direction[2] = vec3<F32>( 0.0f,  0.0f,  1.0f);
    // -z
    _direction[3] = vec3<F32>( 0.0f,  0.0f, -1.0f);
    // +y
    _direction[4] = vec3<F32>( 0.0f,  1.0f,  0.0f);
    // -y
    _direction[5] = vec3<F32>( 0.0f, -1.0f,  0.0f);
    
}

};
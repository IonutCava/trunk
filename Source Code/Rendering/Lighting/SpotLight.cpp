#include "Headers/SpotLight.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "ShadowMapping/Headers/ShadowMap.h"

SpotLight::SpotLight(U8 slot, F32 range) : Light(slot,range,LIGHT_TYPE_SPOT) {
	_properties._attenuation.w = 35;
	_properties._position = vec4<F32>(0,0,5,2.0f);
	_properties._direction = vec4<F32>(1,1,0,0);
	_properties._direction.w = 1;
};

const mat4<F32>& SpotLight::getLightViewMatrix(U8 index){
    _lightViewMatrix.set(GFX_DEVICE.getLookAt(getPosition(), getDirection()));
    return _lightViewMatrix;
}
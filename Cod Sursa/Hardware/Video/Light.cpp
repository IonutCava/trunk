#include "Light.h"
#include "Hardware/Video/GFXDevice.h"

Light::Light(U32 slot) : _slot(slot)
{
	vec2 angle = vec2(0.0f, RADIANS(45.0f));
	vec4 position = vec4(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),	-sinf(angle.x) * sinf(angle.y),	0.0f );
	vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
	vec4 diffuse = white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + cosf(angle.y) * 0.75f);

	_lightProperties["spotDirection"] = vec4(0.0f, 0.0f, 0.0f,0.0f);
	_lightProperties["position"] = position;
	_lightProperties["ambient"] = white;
	_lightProperties["diffuse"] = diffuse;
	_lightProperties["specular"] = diffuse;

	GFXDevice::getInstance().createLight(_slot);
}

void Light::update()
{
	GFXDevice::getInstance().setLight(_slot,_lightProperties);
}

void Light::setLightProperties(const string& name, vec4 values)
{
	tr1::unordered_map<string,vec4>::iterator it = _lightProperties.find(name);
	if (it != _lightProperties.end())
		_lightProperties[name] = values;
}
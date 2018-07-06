#include "Light.h"
#include "Hardware/Video/GFXDevice.h"

Light::Light(U32 slot) : _slot(slot)
{
	vec4 _white(1.0f,1.0f,1.0f,1.0f);
	vec2 angle = vec2(0.0f, RADIANS(45.0f));
	vec4 position = vec4(-cosf(angle.x) * sinf(angle.y),-cosf(angle.y),	-sinf(angle.x) * sinf(angle.y),	0.0f );
	vec4 diffuse = _white.lerp(vec4(1.0f, 0.5f, 0.0f, 1.0f), vec4(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + cosf(angle.y) * 0.75f);

	_lightProperties["position"] = position;
	_lightProperties["ambient"] = _white;
	_lightProperties["diffuse"] = diffuse;
	_lightProperties["specular"] = diffuse;

	GFXDevice::getInstance().createLight(_slot);
}

void Light::update()
{
	GFXDevice::getInstance().setLight(_slot,_lightProperties);
}

void Light::setLightProperties(const std::string& name, vec4 values)
{
	std::tr1::unordered_map<std::string,vec4>::iterator it = _lightProperties.find(name);
	if (it != _lightProperties.end())
		_lightProperties[name] = values;

	if(name.compare("spotDirection") == 0)
		_lightProperties["spotDirection"] = values;
}
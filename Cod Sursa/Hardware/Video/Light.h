#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "resource.h"

class Light
{

public:
	Light(U32 slot);
	void update();
	void setLightProperties(const std::string& name, vec4 values);
	vec4& getDiffuseColor() {return _lightProperties["diffuse"];}
	vec4& getPosition() {return  _lightProperties["position"];}
private:
	std::tr1::unordered_map<std::string,vec4> _lightProperties;
	U32 _slot;
};

#endif
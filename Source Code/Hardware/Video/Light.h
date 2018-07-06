#ifndef DIVIDE_LIGHT_H_
#define DIVIDE_LIGHT_H_

#include "resource.h"

class Sphere3D;
class Light
{

public:
	Light(U8 slot, F32 radius = 1.0f); 
	~Light();
	void update();
	void setLightProperties(const std::string& name, vec4 values);
	vec4& getDiffuseColor() {return _lightProperties["diffuse"];}
	vec4& getPosition() {return  _lightProperties["position"];}
	F32&  getRadius()   {return _radius;}
	void  getWindowRect(U16 & x, U16 & y, U16 & width, U16 & height);
	void  draw();
private:
	std::tr1::unordered_map<std::string,vec4> _lightProperties;
	U8 _slot;
	F32 _radius;
	Sphere3D *_light; //Used for debug rendering -Ionut
};

#endif
#ifndef SKY_H
#define SKY_H

#include "resource.h"
#include "Utility/Headers/Singleton.h"
#include "Utility/Headers/MathClasses.h"
#include "TextureManager/TextureCubemap.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Managers/ResourceManager.h"
#include "Geometry/Predefined/Sphere3D.h"

SINGLETON_BEGIN( Sky ) 

public:
	void draw() const;
	void setParams(const vec3& eyePos,const vec3& sunVect, bool invert, bool drawSun, bool drawSky) ;
	vec3 Sky::getSunVector(){	return _sunVect; }

private:

	bool			  _init,_invert,_drawSky,_drawSun;
	Shader*			  _skyShader;
	TextureCubemap*	  _skybox;
	vec3			  _sunVect,	_eyePos;
	Sphere3D          *_sky,*_sun;
	
private:
	Sky();
	~Sky();
	void drawSky() const;
	void drawSun() const;
	void drawSkyAndSun() const;

SINGLETON_END()

#endif



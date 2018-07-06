#ifndef SKY_H
#define SKY_H

#include "resource.h"
#include "Utility/Headers/Singleton.h"
#include "Utility/Headers/MathClasses.h"
#include "TextureManager/TextureCubemap.h"
#include "Hardware/Video/ShaderHandler.h"
#include "Managers/ResourceManager.h"

SINGLETON_BEGIN( Sky ) 

public:
	void draw() const;
	void setParams(const vec3& eyePos,const vec3& sunVect, bool invert, bool drawSun, bool drawSky) ;
	vec3 Sky::getSunVector(){	return _sunVect; }

private:
	Sky();

	bool      _init,_invert,_drawSky,_drawSun;
	Shader*   _skyShader;
	U32		  _skybox;
	vec3      _sunVect,	_eyePos;
	

	void drawSky() const;
	void drawSun() const;
	void drawSkyAndSun() const;

SINGLETON_END()

#endif



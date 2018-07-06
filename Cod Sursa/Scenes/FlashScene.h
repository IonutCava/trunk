#ifndef _FLASH_SCENE_H
#define _FLASH_SCENE_H

#include "Scene.h"

class FlashScene : public Scene
{
public:
	void render();
	void preRender();
	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}
	void processInput();
	void processEvents(F32 time);

private:
	F32 i ;
	vec2 _sunAngle;
	vec4 _sunVector;
	F32 angleLR,angleUD,moveFB,moveLR,update_time;
	vec4 _vSunColor;
	vector<F32> _eventTimers;

};

#endif
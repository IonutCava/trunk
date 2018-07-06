#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scene.h"
#include "Hardware/Video/FrameBufferObject.h"


class MainScene : public Scene
{

public:
	/*General Scene Requirement*/
	void render();
	void preRender();
	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}
	bool unload();

	

	/*Specific Scene Requirement*/
	void renderActors();
	void processInput();
	void processEvents(F32 time);

private:
	void test(boost::any a, CallbackParam b);
	FrameBufferObject*     _skyFBO;
	vec2 _sunAngle;
	vec4 _sunVector;
	mat4 _matSunModelviewProj;
	F32 angleLR,angleUD,moveFB,moveLR;
	vector<F32> _eventTimers;

};

#endif;
#ifndef _MAIN_SCENE_H
#define _MAIN_SCENE_H

#include "Scene.h"
#include "Hardware/Video/FrameBufferObject.h"


class MainScene : public Scene
{
public:
	/*General Scene Requirement*/
	MainScene(string name, string mainScript);
	void render();
	void preRender();
	bool load(const string& name);
	bool unload();
	void processEvents(F32 time);

	/*Specific Scene Requirement*/
	void renderActors();
	void processInput();
	void processKey(int Key);
	
private:
	FrameBufferObject*     _skyFBO;
	vec2 _sunAngle;
	vec4 _sunVector;
	vec3 _cameraEye;
	mat4 _matSunModelviewProj;
	F32 angleLR,angleUD,moveFB;
	vector<F32> _eventTimers;

};

#endif;
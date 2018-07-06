#ifndef _CUBE_SCENE_H
#define _CUBE_SCENE_H

#include "Scene.h"

class CubeScene : public Scene
{
public:
	CubeScene(string name, string mainScript);
	void render();
	void preRender();
	bool load(const string& name);
	bool unload();
	void processInput();
	void processEvents(F32 time){}

private:
	GFXDevice& _GFX;
	vec3 _cameraEye;
	F32 angleLR,angleUD,moveFB,update_time;
};

#endif
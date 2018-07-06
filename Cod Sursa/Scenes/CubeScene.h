#ifndef _CUBE_SCENE_H
#define _CUBE_SCENE_H

#include "Scene.h"
class Sphere3D;
class CubeScene : public Scene
{
public:
	void render();
	void preRender();
	bool load(const std::string& name);
	bool unload();
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
private:

	F32 angleLR,angleUD,moveFB,moveLR;
	Quad3D*	_renderQuad;
};

#endif
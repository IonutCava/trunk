#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scene.h"

class PingPongScene : public Scene
{

public:
	PingPongScene() {}
	~PingPongScene() {}
	void render();
	void preRender();

	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void OnJoystickMovePOV(const OIS::JoyStickEvent& event,I8 pov);
	void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis);
	void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button);
private:
	void test(boost::any a, CallbackParam b);
	void servesteMingea();

private:
	std::vector<F32> _eventTimers;
	F32 angleLR,angleUD,moveFB,moveLR;
	I8 _scor;
	std::vector<std::string> _quotes;

	Sphere3D* _minge;

};

#endif
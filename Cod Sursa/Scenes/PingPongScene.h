#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scene.h"

class PingPongScene : public Scene
{

public:
	PingPongScene() {}
	~PingPongScene() {_events[0]->stopEvent();  _events.erase(_events.begin()); _events.clear();}
	void render();
	void preRender();

	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);
	void OnJoystickMovePOV(const OIS::JoyStickEvent& event,I32 pov);
	void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I32 axis);
	void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I32 button);
private:
	void test(boost::any a, CallbackParam b);
	void servesteMingea();

private:
	vector<F32> _eventTimers;
	F32 angleLR,angleUD,moveFB,moveLR;
	I32 _scor;
	vector<string> _quotes;

	Sphere3D* _minge;

};

#endif
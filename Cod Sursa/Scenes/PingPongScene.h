#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scene.h"
#include "ASIO.h"

class PingPongScene : public Scene
{

public:
	PingPongScene() : _asio(ASIO::getInstance()){}
	~PingPongScene() {_events[0]->stopEvent();  _events.erase(_events.begin()); _events.clear();}
	void render();
	void preRender();

	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	void processInput();
	void processEvents(F32 time);

private:
	void test(boost::any a, CallbackParam b);
	void servesteMingea();

private:
	vector<F32> _eventTimers;
	F32 angleLR,angleUD,moveFB,moveLR;
	I32 _scor;
	vector<string> _quotes;

	/*Cred ca's utile astea, nu?*/
	ASIO& _asio;
	Sphere3D* _minge;

};

#endif
#ifndef _NETWORK_SCENE_H
#define _NETWORK_SCENE_H

#include "Scene.h"

class NetworkScene : public Scene
{

public:
	void render();
	void preRender();

	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();

	void processEvents(F32 time);
	void processInput();
private:
	vector<F32> _eventTimers;
};

#endif
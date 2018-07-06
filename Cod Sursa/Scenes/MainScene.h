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
	bool load(const std::string& name);
	bool loadResources(bool continueOnErrors);
	bool loadEvents(bool continueOnErrors){return true;}


private:
	/*Specific Scene Requirement*/
	void restoreLightCameraMatrices();
	void setLightCameraMatrices();
	bool updateLights();
	void renderActors();
	void processInput();
	void processEvents(F32 time);
	void test(boost::any a, CallbackParam b);
	void onKeyDown(const OIS::KeyEvent& key);
	void onKeyUp(const OIS::KeyEvent& key);

private:
	
	FrameBufferObject*     _skyFBO,*_depthMap[2];
	vec2 _sunAngle;
	vec4 _sunVector;
	F32  _sun_cosy;
	mat4 _sunModelviewProj;
	F32 angleLR,angleUD,moveFB,moveLR;
	std::vector<F32> _eventTimers;

};

#endif;
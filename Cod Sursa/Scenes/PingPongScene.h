#ifndef _PINGPONG_SCENE_H
#define _PINGPONG_SCENE_H

#include "Scene.h"
#include "ASIO.h"

class Minge : public Sphere3D
{

public:
	Minge(F32 size, U32 resolution) : Sphere3D(size,resolution) {}

	void computeBoundingBox() {_bb.setMin(vec3(getTransform()->getPosition().x - _size,getTransform()->getPosition().y - _size,getTransform()->getPosition().z - _size)); 
							   _bb.setMax(vec3(getTransform()->getPosition().x + _size,getTransform()->getPosition().y + _size,getTransform()->getPosition().z + _size));} 
	void setPosition(vec3 position)
	{
		getTransform()->translate(position);
		computeBoundingBox();
	}

	BoundingBox&   getBoundingBox() {return _bb;}

private:											
	BoundingBox _bb;
};

class PingPongScene : public Scene
{

public:
	PingPongScene() : _asio(ASIO::getInstance()), _returMinge(false) {}
	void render();
	void preRender();

	bool load(const string& name);
	bool loadResources(bool continueOnErrors);
	bool unload();
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
	bool _returMinge;
	ASIO& _asio;
	Minge* _minge;

};

#endif
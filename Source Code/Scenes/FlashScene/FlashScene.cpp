#include "Headers/FlashScene.h"

#include "Rendering/Camera/Headers/Camera.h"
#include "GUI/Headers/GUI.h"

void FlashScene::render(){
	
	_sceneGraph->render();
}


void FlashScene::preRender(){

} 

void FlashScene::processInput(){

	if(_angleLR) _camera->RotateX(_angleLR * Framerate::getInstance().getSpeedfactor());
	if(_angleUD) _camera->RotateY(_angleUD * Framerate::getInstance().getSpeedfactor());
	if(_moveFB)  _camera->MoveForward(_moveFB * (Framerate::getInstance().getSpeedfactor()/5));
	if(_moveLR)	 _camera->MoveStrafe(_moveLR * (Framerate::getInstance().getSpeedfactor()/5));

}

void FlashScene::processEvents(F32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _eventTimers[0] >= FpsDisplay)	{
		
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %5.2f", Framerate::getInstance().getFps());
		_eventTimers[0] += FpsDisplay;
	}
}

bool FlashScene::load(const std::string& name){

	setInitialData();
	bool state = false;
	addDefaultLight();
	state = loadResources(true);	
	state = loadEvents(true);

	return state;
}

bool FlashScene::loadResources(bool continueOnErrors){

	_sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunVector = vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec3<F32>(60,60,0),          //Position
							   BITMAP_8_BY_13,    //Font
							   vec3<F32>(0.0f,0.2f, 1.0f),  //Color
							   "FPS: %s",0);    //Text and arguments
		_eventTimers.push_back(0.0f);
    i = 0;
	return true;
}

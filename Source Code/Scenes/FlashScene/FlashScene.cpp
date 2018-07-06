#include "Headers/FlashScene.h"
#include "Managers/Headers/SceneManager.h"

REGISTER_SCENE(FlashScene);

void FlashScene::render(){
}

void FlashScene::preRender(){
}

void FlashScene::processInput(){
	if(state()->_angleLR) renderState()->getCamera()->RotateX(state()->_angleLR);
	if(state()->_angleUD) renderState()->getCamera()->RotateY(state()->_angleUD );
	if(state()->_moveFB)  renderState()->getCamera()->MoveForward(state()->_moveFB /5);
	if(state()->_moveLR)  renderState()->getCamera()->MoveStrafe(state()->_moveLR /5);
}

void FlashScene::processTasks(const U32 time){
	F32 FpsDisplay = 0.3f;
	if (time - _taskTimers[0] >= FpsDisplay)	{
		GUI::getInstance().modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", Framerate::getInstance().getFps(), Framerate::getInstance().getFrameTime());
		_taskTimers[0] += FpsDisplay;
	}
}

bool FlashScene::load(const std::string& name){
	///Load scene resources
	SCENE_LOAD(name,true,true);
	addDefaultLight();
	addDefaultSky();
	return loadState;
}

bool FlashScene::loadResources(bool continueOnErrors){
	_sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
	_sunvector = vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
							-cosf(_sunAngle.y),
							-sinf(_sunAngle.x) * sinf(_sunAngle.y),
							0.0f );
		GUI::getInstance().addText("fpsDisplay",           //Unique ID
		                       vec2<I32>(60,60),           //Position
							   Font::DIVIDE_DEFAULT,       //Font
							   vec3<F32>(0.0f,0.2f, 1.0f), //Color
							   "FPS: %s",0);    //Text and arguments
		_taskTimers.push_back(0.0f);
    i = 0;
	return true;
}
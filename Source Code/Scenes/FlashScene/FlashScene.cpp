#include "Headers/FlashScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ApplicationTimer.h"

namespace Divide {

REGISTER_SCENE(FlashScene);

void FlashScene::render(){
}

void FlashScene::preRender(){
}

void FlashScene::processInput(const U64 deltaTime){
}

void FlashScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = getSecToMs(0.3);
    if (_guiTimers[0] >= FpsDisplay)	{
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", 
                         ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _guiTimers[0] = 0.0;
    }
}

void FlashScene::processTasks(const U64 deltaTime){

    Scene::processTasks(deltaTime);
}

bool FlashScene::load(const stringImpl& name, CameraManager* const cameraMgr, GUI* const gui){
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);
    addLight(LIGHT_TYPE_DIRECTIONAL);
	_currentSky = addSky(CreateResource<Sky>(ResourceDescriptor("Default Sky")));
    return loadState;
}

bool FlashScene::loadResources(bool continueOnErrors){
    _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    _sunvector = vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y),
                            0.0f );
        _GUI->addText("fpsDisplay",           //Unique ID
                      vec2<I32>(60,60),           //Position
                      Font::DIVIDE_DEFAULT,       //Font
                      vec3<F32>(0.0f,0.2f, 1.0f), //Color
                      "FPS: %s",0);    //Text and arguments
        _guiTimers.push_back(0.0);
    i = 0;
    return true;
}

};
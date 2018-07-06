#include "Headers/FlashScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

void FlashScene::processInput(const U64 deltaTime) {}

void FlashScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _guiTimers[0] = 0.0;
    }
}

void FlashScene::processTasks(const U64 deltaTime) {
    Scene::processTasks(deltaTime);
}

bool FlashScene::load(const stringImpl& name) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    return loadState;
}

bool FlashScene::loadResources(bool continueOnErrors) {
    _sunAngle = vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    _guiTimers.push_back(0.0);
    i = 0;
    return true;
}

void FlashScene::postLoadMainThread() {
    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
        vec2<I32>(60, 60),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec4<U8>(0, 64, 255, 255),  // Colour
        Util::StringFormat("FPS: %d", 0));  // Text and arguments

    Scene::postLoadMainThread();
}

};
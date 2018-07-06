#include "stdafx.h"

#include "Headers/FlashScene.h"

#include "Core/Headers/StringHelper.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ApplicationTimer.h"

namespace Divide {

void FlashScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    Scene::processInput(idx, deltaTimeUS);
}

void FlashScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimersMS[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _guiTimersMS[0] = 0.0;
    }
}

void FlashScene::processTasks(const U64 deltaTimeUS) {
    Scene::processTasks(deltaTimeUS);
}

bool FlashScene::load(const stringImpl& name) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    return loadState;
}

bool FlashScene::loadResources(bool continueOnErrors) {
    _sunAngle = vec2<F32>(0.0f, Angle::to_RADIANS(45.0f));
    _sunvector =
        FColour(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    _guiTimersMS.push_back(0.0);
    i = 0;
    return true;
}

void FlashScene::postLoadMainThread() {
    _GUI->addText("fpsDisplay",                          // Unique ID
        RelativePosition2D(RelativeValue(0.0f, 60.0f),
                           RelativeValue(0.0f, 60.0f)),  // Position
        Font::DIVIDE_DEFAULT,                            // Font
        UColour(0, 64, 255, 255),                        // Colour
        Util::StringFormat("FPS: %d", 0));               // Text and arguments

    Scene::postLoadMainThread();
}

};
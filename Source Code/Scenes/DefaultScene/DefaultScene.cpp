#include "Headers/DefaultScene.h"

#include "GUI/Headers/GUIButton.h"
#include "Managers/Headers/SceneManager.h"

namespace Divide {
DefaultScene::DefaultScene() 
    : Scene("DefaultScene")
{
}

bool DefaultScene::load(const stringImpl& name, GUI* const gui) {
    bool loadState = SCENE_LOAD(name, gui, true, true);
    SceneGraphNode_wptr light = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::DegreesToRadians(45.0f));
    vec3<F32> sunvector(-cosf(sunAngle.x) * sinf(sunAngle.y),
                        -cosf(sunAngle.y),
                        -sinf(sunAngle.x) * sinf(sunAngle.y));
    
    light.lock()->get<PhysicsComponent>()->setPosition(sunvector);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(sunvector, light.lock()->getNode<Light>()->getDiffuseColor());
    return loadState;
}

void DefaultScene::processGUI(const U64 deltaTime) {
    Scene::processGUI(deltaTime);
}

bool DefaultScene::loadResources(bool continueOnErrors) {
    // Replace buttons with nice, animated elements? images?
    vectorImpl<stringImpl> scenes = SceneManager::instance().sceneNameList();

    const vec2<U16>& resolution = _GUI->getDisplayResolution();

    const I32 spacingX = 10;
    const I32 spacingY = 10;
    const I32 numColumns = 3;
    const I32 numRows = to_int(std::ceil(to_float(scenes.size()) / numColumns));
    const I32 btnWidth = 100;
    const I32 btnHeight = 100;
    const I32 windowCenterX = to_int(resolution.width * 0.5f);
    const I32 windowCenterY = to_int(resolution.height * 0.5f);
    const I32 btnStartXOffset = to_int(windowCenterX - numColumns * 0.5f * btnWidth);
    const I32 btnStartYOffset = to_int(windowCenterY - numRows * 0.5f * btnHeight);

    size_t i = 0, j = 0;
    for (const stringImpl& scene : scenes) {
        size_t localOffsetX = btnWidth  * (i % numColumns) + spacingX * (i % numColumns);
        size_t localOffsetY = btnHeight * (j % numRows) + spacingY * (j % numRows);

        GUIButton* btn = _GUI->addButton(_ID_RT("StartScene" + scene), scene,
                                          vec2<I32>(btnStartXOffset + localOffsetX ,
                                                    btnStartYOffset + localOffsetY),
                                          vec2<U32>(btnWidth, btnHeight),
                                          vec3<F32>(0.6f, 0.6f, 0.6f),
                                          DELEGATE_BIND(&DefaultScene::loadScene, this, std::placeholders::_1));

        _buttonToSceneMap[btn->getGUID()] = scene;
        i++;
        if (i > 0 && i % numColumns == 0) {
            j++;
        }
    }


    _GUI->addText(_ID("globalMessage"),
                  vec2<I32>(windowCenterX,
                            windowCenterY + (numRows + 1)* btnHeight),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(128, 64, 64, 255),
                  "");

    _taskTimers.push_back(0.0);

    return true;
}

void DefaultScene::processInput(const U64 deltaTime) {
    if (!_sceneToLoad.empty()) {
        _GUI->modifyText(_ID("globalMessage"),
                         Util::StringFormat("Please wait while scene [ %s ] is loading", _sceneToLoad.c_str()));
        Scene* newScene = SceneManager::instance().load(_sceneToLoad);
        SceneManager::instance().setActiveScene(*newScene);
        _sceneToLoad = "";
    }
}

void DefaultScene::processTasks(const U64 deltaTime) {
    static F32 angle = 0;

    D64 SpinTimer = Time::Milliseconds(16);
    if (_taskTimers[0] >= SpinTimer) {
        angle += 0.0125f;
        if (angle >= 360.0f) {
            angle = 0.0f;
        }
        renderState().getCamera().setYaw(angle);

        _taskTimers[0] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

void DefaultScene::loadScene(I64 btnGUID) {
    _sceneToLoad = _buttonToSceneMap[btnGUID];
    Console::d_printf("Loading scene [ %s ]", _sceneToLoad.c_str());

    GUIButton* selection = _GUI->getGuiElement<GUIButton>(getGUID(), btnGUID);
    selection->setText("Loading ...");
    for (hashMapImpl<I64, stringImpl>::value_type it : _buttonToSceneMap) {
        GUIButton* btn = _GUI->getGuiElement<GUIButton>(getGUID(), it.first);
        if (btn->getGUID() != btnGUID) {
            btn->setActive(false);
            btn->setVisible(false);
        }
    }
}

};

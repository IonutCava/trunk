#include "Headers/DefaultScene.h"

#include "GUI/Headers/GUIButton.h"
#include "Managers/Headers/SceneManager.h"
#include "Rendering/PostFX/Headers/PostFX.h"

#include "Dynamics/Entities/Units/Headers/Player.h"

namespace Divide {
DefaultScene::DefaultScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Scene(context, cache, parent, name)
{
}

bool DefaultScene::load(const stringImpl& name) {
    bool loadState = SCENE_LOAD(name, true, true);
    SceneGraphNode_wptr light = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::to_RADIANS(45.0f));
    vec3<F32> sunvector(-cosf(sunAngle.x) * sinf(sunAngle.y),
                        -cosf(sunAngle.y),
                        -sinf(sunAngle.x) * sinf(sunAngle.y));
    
    light.lock()->get<PhysicsComponent>()->setPosition(sunvector);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(sunvector, light.lock()->getNode<Light>()->getDiffuseColour());

    state().saveLoadDisabled(true);

    return loadState;
}

void DefaultScene::processGUI(const U64 deltaTime) {
    Scene::processGUI(deltaTime);
}

bool DefaultScene::loadResources(bool continueOnErrors) {
    _taskTimers.push_back(0.0);

    return true;
}

void DefaultScene::postLoadMainThread() {
    // Replace buttons with nice, animated elements? images?
    vectorImpl<stringImpl> scenes = _parent.sceneNameList();

    const vec2<U16>& resolution = _GUI->getDisplayResolution();

    const I32 spacingX = 10;
    const I32 spacingY = 10;
    const I32 numColumns = 3;
    const I32 numRows = to_I32(std::ceil(to_F32(scenes.size()) / numColumns));
    const I32 btnWidth = 100;
    const I32 btnHeight = 100;
    const I32 windowCenterX = to_I32(resolution.width * 0.5f);
    const I32 windowCenterY = to_I32(resolution.height * 0.5f);
    const I32 btnStartXOffset = to_I32(windowCenterX - numColumns * 0.5f * btnWidth);
    const I32 btnStartYOffset = to_I32(windowCenterY - numRows * 0.5f * btnHeight);
    const I32 quitButtonWidth = 100;
    const I32 quitButtonHeight = 100;
    const I32 playerButtonHeight = 25;

    size_t i = 0, j = 0;
    for (const stringImpl& scene : scenes) {
        size_t localOffsetX = btnWidth  * (i % numColumns) + spacingX * (i % numColumns);
        size_t localOffsetY = btnHeight * (j % numRows) + spacingY * (j % numRows);

        GUIButton* btn = _GUI->addButton(_ID_RT("StartScene" + scene), scene,
            vec2<I32>(btnStartXOffset + localOffsetX,
                btnStartYOffset + localOffsetY),
            vec2<U32>(btnWidth, btnHeight),
            DELEGATE_BIND(&DefaultScene::loadScene, this, std::placeholders::_1));

        _buttonToSceneMap[btn->getGUID()] = scene;
        i++;
        if (i > 0 && i % numColumns == 0) {
            j++;
        }
    }

    _GUI->addButton(_ID_RT("Quit"), "Quit",
        vec2<I32>(resolution.width - quitButtonWidth * 1.5f, resolution.height - quitButtonHeight * 1.5f),
        vec2<U32>(quitButtonWidth, quitButtonHeight),
        [](I64 btnGUID) {Application::instance().RequestShutdown(); });

    _GUI->addButton(_ID_RT("AddPlayer"), "Add Player",
        vec2<I32>(0, resolution.height - playerButtonHeight * 1.5f),
        vec2<U32>(quitButtonWidth, playerButtonHeight),
        [this](I64 btnGUID) {
            addPlayerInternal(true);
        });

    _GUI->addButton(_ID_RT("RemovePlayer"), "Remove Player",
        vec2<I32>(0, resolution.height - playerButtonHeight * 1.5f - playerButtonHeight * 1.5f),
        vec2<U32>(quitButtonWidth, playerButtonHeight),
        [this](I64 btnGUID) {
            if (_scenePlayers.size() > 1) {
                removePlayerInternal(_scenePlayers.back()->index());
            }
    });

    _GUI->addText(_ID("globalMessage"),
        vec2<I32>(windowCenterX,
            windowCenterY + (numRows + 1)* btnHeight),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(128, 64, 64, 255),
        "");
    
    Scene::postLoadMainThread();
}

void DefaultScene::processInput(U8 playerIndex, const U64 deltaTime) {
    if (!_sceneToLoad.empty()) {
        _GUI->modifyText(_ID("globalMessage"),
                         Util::StringFormat("Please wait while scene [ %s ] is loading", _sceneToLoad.c_str()));
        _parent.switchScene(_sceneToLoad, false);
        _sceneToLoad.clear();
    }

    Angle::DEGREES<F32>& angle = _camAngle[getSceneIndexForPlayer(playerIndex)];
    if (playerIndex % 3 == 1) {
        getPlayerForIndex(playerIndex)->getCamera().rotatePitch(angle);
    } else if (playerIndex % 3 == 2) {
        getPlayerForIndex(playerIndex)->getCamera().rotateRoll(angle);
    } else {
        getPlayerForIndex(playerIndex)->getCamera().rotateYaw(angle);
    }
    angle = 0.0f;

    Scene::processInput(playerIndex, deltaTime);
}

void DefaultScene::processTasks(const U64 deltaTime) {
    D64 SpinTimer = Time::Milliseconds(16);
    if (_taskTimers[0] >= SpinTimer) {
        for (hashMapImpl<U8, Angle::DEGREES<F32>>::value_type& it : _camAngle) {
            it.second = 0.25f * ((it.first * 2.0f) + 1.0f) * (it.first % 2 == 0 ? -1 : 1);
        }

        _taskTimers[0] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

void DefaultScene::loadScene(I64 btnGUID) {
    PostFX::instance().setFadeOutIn(vec4<U8>(0), 750, 1500, 2000);

    _sceneToLoad = _buttonToSceneMap[btnGUID];
    Console::d_printf("Loading scene [ %s ]", _sceneToLoad.c_str());

    GUIButton* selection = _GUI->getGUIElement<GUIButton>(btnGUID);
    selection->setText("Loading ...");
    for (hashMapImpl<I64, stringImpl>::value_type it : _buttonToSceneMap) {
        GUIButton* btn = _GUI->getGUIElement<GUIButton>(it.first);
        if (btn->getGUID() != btnGUID) {
            btn->setActive(false);
            btn->setVisible(false);
        }
    }
}

void DefaultScene::onSetActive() {
    vectorImpl<stringImpl> scenes = _parent.sceneNameList();

    for (const stringImpl& scene : scenes) {
        GUIButton* btn = _GUI->getGUIElement<GUIButton>(_ID_RT("StartScene" + scene));
        
        btn->setText(scene);
        btn->setActive(true);
        btn->setVisible(true);
    }

    Scene::onSetActive();
}

};

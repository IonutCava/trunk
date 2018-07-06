#include "stdafx.h"

#include "Headers/DefaultScene.h"

#include "GUI/Headers/GUIButton.h"

#include "Core/Headers/PlatformContext.h"
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
    SceneGraphNode* light = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::to_RADIANS(45.0f));
    vec3<F32> sunvector(-cosf(sunAngle.x) * sinf(sunAngle.y),
                        -cosf(sunAngle.y),
                        -sinf(sunAngle.x) * sinf(sunAngle.y));
    
    light->get<TransformComponent>()->setPosition(sunvector);
    PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
    constants.set("enable_sun", PushConstantType::BOOL, true);
    constants.set("sun_vector", PushConstantType::VEC3, sunvector);
    constants.set("sun_colour", PushConstantType::VEC3, light->getNode<Light>()->getDiffuseColour());

    state().saveLoadDisabled(true);

    return loadState;
}

void DefaultScene::processGUI(const U64 deltaTimeUS) {
    Scene::processGUI(deltaTimeUS);
}

bool DefaultScene::loadResources(bool continueOnErrors) {
    _taskTimers.push_back(0.0);

    return true;
}

void DefaultScene::postLoadMainThread() {
    // Replace buttons with nice, animated elements? images?
    const vectorImpl<stringImpl>& scenes = _parent.sceneNameList();

    const vec2<U16>& resolution = _context.gfx().renderingResolution();

    const I32 spacingX = 10;
    const I32 spacingY = 10;
    const I32 numColumns = 3;
    const I32 numRows = to_I32(std::ceil(to_F32(scenes.size()) / numColumns));
    const I32 btnWidth = 100;
    const I32 btnHeight = 100;
    const I32 windowCenterX = to_I32(resolution.width * 0.5f);
    const I32 windowCenterY = to_I32(resolution.height * 0.5f);
    const F32 btnStartXOffset = NORMALIZE(to_F32(windowCenterX) - numColumns * 0.5f * btnWidth, 0.0f, to_F32(resolution.width));
    const F32 btnStartYOffset = NORMALIZE(to_F32(windowCenterY) - numRows * 0.5f * btnHeight, 0.0f, to_F32(resolution.height));
    const I32 quitButtonWidth = 100;
    const I32 quitButtonHeight = 100;
    const I32 playerButtonHeight = 25;

    RelativePosition2D buttonPosition(RelativeValue(btnStartXOffset, 0.0f),
                                      RelativeValue(btnStartYOffset, 0.0f));
    // In pixels
    RelativeScale2D buttonSize(RelativeValue(0.0f, to_F32(btnWidth)),
                               RelativeValue(0.0f, to_F32(btnHeight)));

    size_t i = 0, j = 0;
    for (const stringImpl& scene : scenes) {
        size_t localOffsetX = btnWidth  * (i % numColumns) + spacingX * (i % numColumns);
        size_t localOffsetY = btnHeight * (j % numRows) + spacingY * (j % numRows);

        buttonPosition.d_x.d_offset = to_F32(localOffsetX);
        buttonPosition.d_y.d_offset = to_F32(localOffsetY);
        GUIButton* btn = _GUI->addButton(_ID_RT("StartScene" + scene), scene,
                                         buttonPosition,
                                         buttonSize,
                                         DELEGATE_BIND(&DefaultScene::loadScene, this, std::placeholders::_1));

        _buttonToSceneMap[btn->getGUID()] = scene;
        i++;
        if (i > 0 && i % numColumns == 0) {
            j++;
        }
    }

    RelativePosition2D quitPosition(RelativeValue(0.0f, resolution.width - quitButtonWidth * 1.5f),
                                    RelativeValue(0.0f, resolution.height - quitButtonHeight * 1.5f));
    RelativeScale2D quitScale(RelativeValue(0.0f, to_F32(quitButtonWidth)),
                              RelativeValue(0.0f, to_F32(quitButtonHeight)));

    _GUI->addButton(_ID_RT("Quit"), "Quit",
                    quitPosition,
                    quitScale,
                    [this](I64 btnGUID) {
                        _context.app().RequestShutdown();
                    });

    RelativePosition2D playerChangePosition(RelativeValue(0.01f, 0.0f),
                                            RelativeValue(0.0f, resolution.height - playerButtonHeight * 1.5f));

    RelativeScale2D playerChangeScale = pixelScale(quitButtonWidth, playerButtonHeight);

    _GUI->addButton(_ID_RT("AddPlayer"), "Add Player",
                    playerChangePosition,
                    playerChangeScale,
                    [this](I64 btnGUID) {
                        addPlayerInternal(true);
                    });

    playerChangePosition.d_y.d_offset -= playerButtonHeight * 1.5f;

    _GUI->addButton(_ID_RT("RemovePlayer"), "Remove Player",
                    playerChangePosition,
                    playerChangeScale,
                    [this](I64 btnGUID) {
                        if (_scenePlayers.size() > 1) {
                            removePlayerInternal(_scenePlayers.back()->index());
                        }
                });

    RelativePosition2D textPosition = pixelPosition(windowCenterX, windowCenterY + (numRows + 1)* btnHeight);
    _GUI->addText(_ID("globalMessage"),
                  textPosition,
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(128, 64, 64, 255),
                  "");
    
    Scene::postLoadMainThread();
}

void DefaultScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    if (!_sceneToLoad.empty()) {
        _GUI->modifyText(_ID("globalMessage"),
                         Util::StringFormat("Please wait while scene [ %s ] is loading", _sceneToLoad.c_str()));
        _parent.switchScene(_sceneToLoad, false);
        _sceneToLoad.clear();
    }

    Angle::DEGREES<F32>& angle = _camAngle[getSceneIndexForPlayer(idx)];
    if (idx % 3 == 1) {
        getPlayerForIndex(idx)->getCamera().rotatePitch(angle);
    } else if (idx % 3 == 2) {
        getPlayerForIndex(idx)->getCamera().rotateRoll(angle);
    } else {
        getPlayerForIndex(idx)->getCamera().rotateYaw(angle);
    }
    angle = 0.0f;

    Scene::processInput(idx, deltaTimeUS);
}

void DefaultScene::processTasks(const U64 deltaTimeUS) {
    D64 SpinTimer = Time::Milliseconds(16.0);
    if (_taskTimers[0] >= SpinTimer) {
        for (hashMapImpl<U8, Angle::DEGREES<F32>>::value_type& it : _camAngle) {
            it.second = 0.25f * ((it.first * 2.0f) + 1.0f) * (it.first % 2 == 0 ? -1 : 1);
        }

        _taskTimers[0] = 0.0;
    }

    Scene::processTasks(deltaTimeUS);
}

void DefaultScene::loadScene(I64 btnGUID) {
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

#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

void MainScene::updateLights() {
    if (!_updateLights) return;

    _sun_cosy = cosf(_sunAngle.y);
    _sunColor =
        Lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
             vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + _sun_cosy * 0.75f);

    _sun.lock()->get<PhysicsComponent>()->setPosition(_sunvector);
    _sun.lock()->getNode<Light>()->setDiffuseColor(_sunColor);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(_sunvector, _sunColor);

    _updateLights = false;
    return;
}

void MainScene::processInput(const U64 deltaTime) {
    if (state().cameraUpdated()) {
        Camera& cam = renderState().getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        if (!_freeflyCamera) {
            F32 terrainHeight = 0.0f;
            vec3<F32> eyePosition = cam.getEye();
            for (SceneGraphNode_wptr terrainNode : _visibleTerrains) {
                Terrain* ter = terrainNode.lock()->getNode<Terrain>();
                assert(ter != nullptr);
                CLAMP<F32>(eyePosition.x,
                           ter->getDimensions().width * 0.5f * -1.0f,
                           ter->getDimensions().width * 0.5f);
                CLAMP<F32>(eyePosition.z,
                           ter->getDimensions().height * 0.5f * -1.0f,
                           ter->getDimensions().height * 0.5f);

                terrainHeight =
                    ter->getPositionFromGlobal(eyePosition.x, eyePosition.z).y;
                if (!IS_ZERO(terrainHeight)) {
                    eyePosition.y = terrainHeight + 1.85f;
                    cam.setEye(eyePosition);
                    break;
                }
            }
            _GUI->modifyText(_ID("camPosition"),
                             Util::StringFormat("[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f] [TerHght: %5.2f ]",
                                                 eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw, terrainHeight));
        } else {
            _GUI->modifyText(_ID("camPosition"),
                             Util::StringFormat("[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
                                                eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));
        }
    }
}

void MainScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.5);
    D64 TimeDisplay = Time::SecondsToMilliseconds(1.0);

    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                             Time::ApplicationTimer::instance().getFps(),
                                             Time::ApplicationTimer::instance().getFrameTime()));
        _GUI->modifyText(_ID("underwater"),
                         Util::StringFormat("Underwater [ %s ] | WaterLevel [%f] ]",
                                             state().cameraUnderwater() ? "true" : "false",
                                             state().waterLevel()));
        _GUI->modifyText(_ID("RenderBinCount"),
                         Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                                            GFX_RENDER_BIN_SIZE, GFX_HIZ_CULL_COUNT));
        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= TimeDisplay) {
        _GUI->modifyText(_ID("timeDisplay"),
                         Util::StringFormat("Elapsed time: %5.0f", Time::ElapsedSeconds()));
        _guiTimers[1] = 0.0;
    }

    Scene::processGUI(deltaTime);
}

void MainScene::processTasks(const U64 deltaTime) {
    updateLights();
    D64 SunDisplay = Time::SecondsToMilliseconds(1.50);
    if (_taskTimers[0] >= SunDisplay) {
        _sunAngle.y += 0.0005f;
        _sunvector = vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                               -cosf(_sunAngle.y),
                               -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);
        _taskTimers[0] = 0.0;
        _updateLights = true;
    }

    Scene::processTasks(deltaTime);
}

bool MainScene::load(const stringImpl& name, GUI* const gui) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    renderState().getCamera().setMoveSpeedFactor(10.0f);

    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _sun.lock()->getNode<DirectionalLight>()->csmSplitCount(3);  // 3 splits
    _sun.lock()->getNode<DirectionalLight>()->csmSplitLogFactor(0.965f);
    _sun.lock()->getNode<DirectionalLight>()->csmNearClipOffset(25.0f);
    _currentSky = addSky();

    for (U8 i = 0; i < _terrainList.size(); i++) {
        SceneGraphNode_ptr terrainNode(_sceneGraph->findNode(_terrainList[i]).lock());
        if (terrainNode) {  // We might have an unloaded terrain in the Array,
                            // and thus, not present in the graph
            Terrain* tempTerrain = terrainNode->getNode<Terrain>();
            if (terrainNode->isActive()) {
                tempTerrain->toggleBoundingBoxes();
                _visibleTerrains.push_back(terrainNode);
            }
        } else {
            Console::errorfn(Locale::get(_ID("ERROR_MISSING_TERRAIN")), _terrainList[i].c_str());
        }
    }

    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    /*ResourceDescriptor infiniteWater("waterEntity");
    _water = CreateResource<WaterPlane>(infiniteWater);
    _water->setParams(50.0f, vec2<F32>(10.0f, 10.0f), vec2<F32>(0.1f, 0.1f),
                      0.34f);
    _waterGraphNode = _sceneGraph->getRoot().addNode(*_water, normalMask);
    SceneGraphNode_ptr waterGraphNode(_waterGraphNode.lock());
    waterGraphNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    waterGraphNode->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);
    // Render the scene for water reflection FB generation
    _water->setReflectionCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
                                                &SceneManager::instance(),
                                                RenderStage::REFLECTION, true, 0));
    _water->setRefractionCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
                                                &SceneManager::instance(),
                                                RenderStage::DISPLAY, true, 0));*/
    return loadState;
}

U16 MainScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {SFX_DEVICE.playSound(_beep);});
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_X, actions);
    actionID++;
    

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _musicPlaying = !_musicPlaying;
        if (_musicPlaying) {
            SceneState::MusicPlaylist::const_iterator it;
            it = state().backgroundMusic().find(_ID("generalTheme"));
            if (it != std::end(state().backgroundMusic())) {
                SFX_DEVICE.playMusic(it->second);
            }
        }
        else {
            SFX_DEVICE.stopMusic();
        }
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_M, actions);
    actionID++;


    _input->actionList().registerInputAction(actionID, [this](InputParams param) { _water->togglePreviewReflection(); });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_R, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _freeflyCamera = !_freeflyCamera;
        renderState().getCamera().setMoveSpeedFactor(_freeflyCamera ? 20.0f
            : 10.0f);
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        for (SceneGraphNode_wptr ter : _visibleTerrains) {
            ter.lock()->getNode<Terrain>()->toggleBoundingBoxes();
        }
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_T, actions);

    return actionID++;
}

bool MainScene::unload() {
    SFX_DEVICE.stopMusic();
    SFX_DEVICE.stopAllSounds();
    RemoveResource(_beep);
    return Scene::unload();
}

void MainScene::test(const std::atomic_bool& stopRequested, cdiggins::any a, CallbackParam b) {
    while (!stopRequested) {
        static bool switchAB = false;
        vec3<F32> pos;
        SceneGraphNode_ptr boxNode(_sceneGraph->findNode("box").lock());

        Object3D* box = nullptr;
        if (boxNode) {
            box = boxNode->getNode<Object3D>();
        }
        if (box) {
            pos = boxNode->get<PhysicsComponent>()->getPosition();
        }

        if (!switchAB) {
            if (pos.x < 300 && pos.z == 0) pos.x++;
            if (pos.x == 300) {
                if (pos.y < 800 && pos.z == 0) pos.y++;
                if (pos.y == 800) {
                    if (pos.z > -500) pos.z--;
                    if (pos.z == -500) switchAB = true;
                }
            }
        } else {
            if (pos.x > -300 && pos.z == -500) pos.x--;
            if (pos.x == -300) {
                if (pos.y > 100 && pos.z == -500) pos.y--;
                if (pos.y == 100) {
                    if (pos.z < 0) pos.z++;
                    if (pos.z == 0) switchAB = false;
                }
            }
        }
        if (box) boxNode->get<PhysicsComponent>()->setPosition(pos);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

bool MainScene::loadResources(bool continueOnErrors) {
    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  Util::StringFormat("FPS: %d", 0));  // Text and arguments

    _GUI->addText(_ID("timeDisplay"), vec2<I32>(60, 80), Font::DIVIDE_DEFAULT,
                  vec4<U8>(164, 64, 64, 255),
                  Util::StringFormat("Elapsed time: %5.0f", Time::ElapsedSeconds()));
    _GUI->addText(_ID("underwater"), vec2<I32>(60, 115), Font::DIVIDE_DEFAULT,
                  vec4<U8>(64, 200, 64, 255),
                  Util::StringFormat("Underwater [ %s ] | WaterLevel [%f] ]", "false", 0));
    _GUI->addText(_ID("RenderBinCount"), vec2<I32>(60, 135), Font::BATANG,
                  vec4<U8>(164, 64, 64, 255),
                  Util::StringFormat("Number of items in Render Bin: %d", 0));
    _taskTimers.push_back(0.0);  // Sun
    _guiTimers.push_back(0.0);  // Fps
    _guiTimers.push_back(0.0);  // Time

    _sunAngle = vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    TaskHandle boxMove(CreateTask(getGUID(),
                               DELEGATE_BIND(&MainScene::test,
                               this,
                               std::placeholders::_1,
                               stringImpl("test"),
                               CallbackParam::TYPE_STRING)));
    boxMove.startTask();
    registerTask(boxMove);

    ResourceDescriptor backgroundMusic("background music");
    backgroundMusic.setResourceLocation(
        _paramHandler.getParam<stringImpl>(_ID("assetsLocation")) +
        "/music/background_music.ogg");
    backgroundMusic.setFlag(true);

    ResourceDescriptor beepSound("beep sound");
    beepSound.setResourceLocation(
        _paramHandler.getParam<stringImpl>(_ID("assetsLocation")) +
        "/sounds/beep.wav");
    beepSound.setFlag(false);
    hashAlg::emplace(state().backgroundMusic(), _ID("generalTheme"),
                     CreateResource<AudioDescriptor>(backgroundMusic));
    _beep = CreateResource<AudioDescriptor>(beepSound);

    const vec3<F32>& eyePos = renderState().getCamera().getEye();
    const vec3<F32>& euler = renderState().getCamera().getEuler();
    _GUI->addText(_ID("camPosition"), vec2<I32>(60, 100), Font::DIVIDE_DEFAULT,
                  vec4<U8>(64, 200, 64, 255),
                  Util::StringFormat("Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
                  eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));

    return true;
}

};

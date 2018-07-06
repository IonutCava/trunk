#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

REGISTER_SCENE(MainScene);

void MainScene::updateLights() {
    if (!_updateLights) return;

    _sun_cosy = cosf(_sunAngle.y);
    _sunColor =
        Lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
             vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + _sun_cosy * 0.75f);

    _sun->setDirection(_sunvector);
    _sun->setDiffuseColor(_sunColor);
    _currentSky->getNode<Sky>()->setSunProperties(_sunvector, _sunColor);
    for (SceneGraphNode* const ter : _visibleTerrains) {
        ter->getComponent<RenderingComponent>()
            ->getMaterialInstance()
            ->setAmbient(_sunColor);
    }

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
            for (SceneGraphNode* const terrainNode : _visibleTerrains) {
                Terrain* ter = terrainNode->getNode<Terrain>();
                assert(ter != nullptr);
                CLAMP<F32>(eyePosition.x,
                           ter->getDimensions().width * 0.5 * -1.0f,
                           ter->getDimensions().width * 0.5f);
                CLAMP<F32>(eyePosition.z,
                           ter->getDimensions().height * 0.5 * -1.0f,
                           ter->getDimensions().height * 0.5f);

                terrainHeight =
                    ter->getPositionFromGlobal(eyePosition.x, eyePosition.z).y;
                if (!IS_ZERO(terrainHeight)) {
                    eyePosition.y = terrainHeight + 1.85f;
                    cam.setEye(eyePosition);
                    break;
                }
            }
            _GUI->modifyText("camPosition",
                             "[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f "
                             "| Yaw: %5.2f] [TerHght: %5.2f ]",
                             eyePos.x, eyePos.y, eyePos.z, euler.pitch,
                             euler.yaw, terrainHeight);
        } else {
            _GUI->modifyText("camPosition",
                             "[ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f "
                             "| Yaw: %5.2f]",
                             eyePos.x, eyePos.y, eyePos.z, euler.pitch,
                             euler.yaw);
        }
    }
}

void MainScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.5);
    D32 TimeDisplay = Time::SecondsToMilliseconds(1.0);

    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f",
                         Time::ApplicationTimer::getInstance().getFps(),
                         Time::ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("underwater", "Underwater [ %s ] | WaterLevel [%f] ]",
                         state().cameraUnderwater() ? "true" : "false",
                         state().waterLevel());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d",
                         GFX_RENDER_BIN_SIZE);
        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= TimeDisplay) {
        _GUI->modifyText("timeDisplay", "Elapsed time: %5.0f",
                         Time::ElapsedSeconds());
        _guiTimers[1] = 0.0;
    }

    Scene::processGUI(deltaTime);
}

void MainScene::processTasks(const U64 deltaTime) {
    updateLights();
    D32 SunDisplay = Time::SecondsToMilliseconds(1.50);
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

    _sun = addLight(LightType::DIRECTIONAL,
               GET_ACTIVE_SCENEGRAPH().getRoot()).getNode<DirectionalLight>();
    _sun->csmSplitCount(3);  // 3 splits
    _sun->csmSplitLogFactor(0.965f);
    _sun->csmNearClipOffset(25.0f);
    _currentSky =
        &addSky(CreateResource<Sky>(ResourceDescriptor("Default Sky")));

    for (U8 i = 0; i < _terrainInfoArray.size(); i++) {
        SceneGraphNode* terrainNode = _sceneGraph.findNode(
            _terrainInfoArray[i]->getVariable("terrainName"));
        if (terrainNode) {  // We might have an unloaded terrain in the Array,
                            // and thus, not present in the graph
            Terrain* tempTerrain = terrainNode->getNode<Terrain>();
            if (terrainNode->isActive()) {
                tempTerrain->toggleBoundingBoxes();
                _visibleTerrains.push_back(terrainNode);
            }
        } else {
            Console::errorfn(Locale::get("ERROR_MISSING_TERRAIN"),
                             _terrainInfoArray[i]->getVariable("terrainName"));
        }
    }

    ResourceDescriptor infiniteWater("waterEntity");
    _water = CreateResource<WaterPlane>(infiniteWater);
    _water->setParams(50.0f, vec2<F32>(10.0f, 10.0f), vec2<F32>(0.1f, 0.1f),
                      0.34f);
    _waterGraphNode = &_sceneGraph.getRoot().addNode(*_water);
    _waterGraphNode->useDefaultTransform(false);
    _waterGraphNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    _waterGraphNode->getComponent<NavigationComponent>()->navigationContext(
        NavigationComponent::NavigationContext::NODE_IGNORE);
    // Render the scene for water reflection FB generation
    _water->setReflectionCallback(DELEGATE_BIND(
        &SceneManager::renderVisibleNodes, &SceneManager::getInstance(), true));
    _water->setRefractionCallback(DELEGATE_BIND(
        &SceneManager::renderVisibleNodes, &SceneManager::getInstance(), true));

    SceneInput::PressReleaseActions cbks;
    cbks.second = DELEGATE_BIND(&SFXDevice::playSound, &SFX_DEVICE, _beep);
    _input->addKeyMapping(Input::KeyCode::KC_X, cbks);

    cbks.second = [this]() {
        _musicPlaying = !_musicPlaying;
        if (_musicPlaying) {
            SceneState::MusicPlaylist::const_iterator it;
            it = state().backgroundMusic().find("generalTheme");
            if (it != std::end(state().backgroundMusic())) {
                SFX_DEVICE.playMusic(it->second);
            }
        } else {
            SFX_DEVICE.stopMusic();
        }
    };

    _input->addKeyMapping(Input::KeyCode::KC_M, cbks);

    cbks.second = [this]() { _water->togglePreviewReflection(); };
    _input->addKeyMapping(Input::KeyCode::KC_R, cbks);

    cbks.second = [this]() {

        _freeflyCamera = !_freeflyCamera;
        renderState().getCamera().setMoveSpeedFactor(_freeflyCamera ? 20.0f
                                                                    : 10.0f);
    };

    _input->addKeyMapping(Input::KeyCode::KC_F, cbks);
    cbks.second = [this]() {
        for (SceneGraphNode* const ter : _visibleTerrains) {
            ter->getNode<Terrain>()->toggleBoundingBoxes();
        }
    };

    _input->addKeyMapping(Input::KeyCode::KC_T, cbks);

    return loadState;
}

bool MainScene::unload() {
    SFX_DEVICE.stopMusic();
    SFX_DEVICE.stopAllSounds();
    RemoveResource(_beep);
    return Scene::unload();
}

void MainScene::test(cdiggins::any a, CallbackParam b) {
    static bool switchAB = false;
    vec3<F32> pos;
    SceneGraphNode* boxNode = _sceneGraph.findNode("box");
    Object3D* box = nullptr;
    if (boxNode) box = boxNode->getNode<Object3D>();
    if (box) pos = boxNode->getComponent<PhysicsComponent>()->getPosition();

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
    if (box) boxNode->getComponent<PhysicsComponent>()->setPosition(pos);
}

bool MainScene::loadResources(bool continueOnErrors) {
    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  "FPS: %s", 0);  // Text and arguments

    _GUI->addText("timeDisplay", vec2<I32>(60, 80), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f), "Elapsed time: %5.0f",
                  Time::ElapsedSeconds());
    _GUI->addText("underwater", vec2<I32>(60, 115), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Underwater [ %s ] | WaterLevel [%f] ]", "false", 0);
    _GUI->addText("RenderBinCount", vec2<I32>(60, 135), Font::BATANG,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  "Number of items in Render Bin: %d", 0);
    _taskTimers.push_back(0.0);  // Sun
    _guiTimers.push_back(0.0);  // Fps
    _guiTimers.push_back(0.0);  // Time

    _sunAngle = vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    Kernel& kernel = Application::getInstance().getKernel();

    Task_ptr boxMove(
        kernel.AddTask(Time::MillisecondsToMicroseconds(30), 0,
                       DELEGATE_BIND(&MainScene::test, this, stringImpl("test"),
                                     CallbackParam::TYPE_STRING)));
    registerTask(boxMove);
    boxMove->startTask();

    ResourceDescriptor backgroundMusic("background music");
    backgroundMusic.setResourceLocation(
        _paramHandler.getParam<stringImpl>("assetsLocation") +
        "/music/background_music.ogg");
    backgroundMusic.setFlag(true);

    ResourceDescriptor beepSound("beep sound");
    beepSound.setResourceLocation(
        _paramHandler.getParam<stringImpl>("assetsLocation") +
        "/sounds/beep.wav");
    beepSound.setFlag(false);
    hashAlg::emplace(state().backgroundMusic(), stringImpl("generalTheme"),
                     CreateResource<AudioDescriptor>(backgroundMusic));
    _beep = CreateResource<AudioDescriptor>(beepSound);

    const vec3<F32>& eyePos = renderState().getCamera().getEye();
    const vec3<F32>& euler = renderState().getCamera().getEuler();
    _GUI->addText("camPosition", vec2<I32>(60, 100), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | "
                  "Yaw: %5.2f]",
                  eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);

    return true;
}

};
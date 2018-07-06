#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {
MainScene::MainScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
   : Scene(context, cache, parent, name),
    _water(nullptr),
    _beep(nullptr),
    _freeflyCamera(false),
    _updateLights(true),
    _musicPlaying(false),
    _sun_cosy(0.0f)
{
}

void MainScene::updateLights() {
    if (!_updateLights) return;

    _sun_cosy = cosf(_sunAngle.y);
    _sunColour =
        Lerp(vec4<F32>(1.0f, 0.5f, 0.0f, 1.0f),
             vec4<F32>(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + _sun_cosy * 0.75f);

    _sun.lock()->get<PhysicsComponent>()->setPosition(_sunvector);
    _sun.lock()->getNode<Light>()->setDiffuseColour(_sunColour);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(_sunvector, _sunColour);

    _updateLights = false;
    return;
}

void MainScene::processInput(U8 playerIndex, const U64 deltaTime) {
    if (state().playerState(playerIndex).cameraUpdated()) {
        Camera& cam = getPlayerForIndex(playerIndex)->getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        if (!_freeflyCamera) {
            F32 terrainHeight = 0.0f;
            vec3<F32> eyePosition = cam.getEye();
            for (SceneGraphNode_wptr terrainNode : _visibleTerrains) {
                const std::shared_ptr<Terrain>& ter = terrainNode.lock()->getNode<Terrain>();
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

    Scene::processInput(playerIndex, deltaTime);
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
                                             state().playerState(0).cameraUnderwater() ? "true" : "false",
                                             state().waterLevel()));
        _GUI->modifyText(_ID("RenderBinCount"),
                         Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                                            _context.gfx().parent().renderPassManager().getLastTotalBinSize(RenderStage::DISPLAY),
                                            _context.gfx().getLastCullCount()));
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

bool MainScene::load(const stringImpl& name) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    Camera::activeCamera()->setMoveSpeedFactor(10.0f);

    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _sun.lock()->getNode<DirectionalLight>()->csmSplitCount(3);  // 3 splits
    _sun.lock()->getNode<DirectionalLight>()->csmSplitLogFactor(0.965f);
    _sun.lock()->getNode<DirectionalLight>()->csmNearClipOffset(25.0f);
    _currentSky = addSky();

    for (U8 i = 0; i < _terrainList.size(); i++) {
        SceneGraphNode_ptr terrainNode(_sceneGraph->findNode(_terrainList[i]).lock());
        if (terrainNode) {  // We might have an unloaded terrain in the Array,
                            // and thus, not present in the graph
            const std::shared_ptr<Terrain>& tempTerrain = terrainNode->getNode<Terrain>();
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
    infiniteWater.setID(to_uint(renderState().getCameraConst().getZPlanes().y));
    _water = CreateResource<WaterPlane>(_resCache, infiniteWater);
    _water->setParams(50.0f, vec2<F32>(10.0f, 10.0f), vec2<F32>(0.1f, 0.1f),
                      0.34f);
    _waterGraphNode = _sceneGraph->getRoot().addNode(_water, normalMask);
    SceneGraphNode_ptr waterGraphNode(_waterGraphNode.lock());
    waterGraphNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    waterGraphNode->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);
    waterGraphNode->get<PhysicsComponent>()->setPositionY(state().waterLevel);

    // Render the scene for water reflection FB generation (Outdated)
    //_water->setReflectionCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
    //                                            &SceneManager::instance(),
    //                                            RenderStage::REFLECTION, true, 0));
    //_water->setRefractionCallback(DELEGATE_BIND(&SceneManager::renderVisibleNodes,
    //                                            &SceneManager::instance(),
    //                                            RenderStage::DISPLAY, true, 0));*/
    return loadState;
}

U16 MainScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {_context.sfx().playSound(_beep);});
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_X, actions);
    actionID++;
    

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _musicPlaying = !_musicPlaying;
        if (_musicPlaying) {
            SceneState::MusicPlaylist::const_iterator it;
            it = state().music(MusicType::TYPE_BACKGROUND).find(_ID("themeSong"));
            if (it != std::cend(state().music(MusicType::TYPE_BACKGROUND))) {
                _context.sfx().playMusic(it->second);
            }
        } else {
            _context.sfx().stopMusic();
        }
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_M, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _freeflyCamera = !_freeflyCamera;
        Camera::activeCamera()->setMoveSpeedFactor(_freeflyCamera ? 20.0f : 10.0f);
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
    _context.sfx().stopMusic();
    _context.sfx().stopAllSounds();

    return Scene::unload();
}

void MainScene::test(const Task& parentTask, cdiggins::any a, CallbackParam b) {
    while (!parentTask.stopRequested()) {
        static bool switchAB = false;
        vec3<F32> pos;
        SceneGraphNode_ptr boxNode(_sceneGraph->findNode("box").lock());

        std::shared_ptr<Object3D> box;
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

    ResourceDescriptor beepSound("beep sound");
    beepSound.setResourceName("beep.wav");
    beepSound.setResourceLocation(Paths::g_assetsLocation + Paths::g_soundsLocation);
    beepSound.setFlag(false);
    _beep = CreateResource<AudioDescriptor>(_resCache, beepSound);

    return true;
}

void MainScene::postLoadMainThread() {
    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
        vec2<I32>(60, 60),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec3<F32>(0.0f, 0.2f, 1.0f),  // Colour
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

    const vec3<F32>& eyePos = Camera::activeCamera()->getEye();
    const vec3<F32>& euler = Camera::activeCamera()->getEuler();
    _GUI->addText(_ID("camPosition"), vec2<I32>(60, 100), Font::DIVIDE_DEFAULT,
        vec4<U8>(64, 200, 64, 255),
        Util::StringFormat("Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
            eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));

    Scene::postLoadMainThread();
}

};

#include "stdafx.h"

#include "Headers/MainScene.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Environment/Water/Headers/Water.h"
#include "Geometry/Material/Headers/Material.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Platform/File/Headers/FileManagement.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

namespace Divide {

namespace {
    vec3<F32> g_waterDimensions(500, 500, 500);
    I64 g_boxMoveTaskID = 0;
};

MainScene::MainScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
   : Scene(context, cache, parent, name),
    _beep(nullptr),
    _freeflyCamera(true/*false*/),
    _updateLights(true),
    _musicPlaying(false),
    _sun_cosy(0.0f)
{
}

void MainScene::updateLights() {
    if (!_updateLights) return;

    _sun_cosy = cosf(_sunAngle.y);
    _sunColour =
        Lerp(FColour(1.0f, 0.5f, 0.0f, 1.0f),
             FColour(1.0f, 1.0f, 0.8f, 1.0f), 0.25f + _sun_cosy * 0.75f);

    _sun->get<TransformComponent>()->setPosition(_sunvector);
    _sun->getNode<Light>()->setDiffuseColour(_sunColour);

    PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
    constants.set("enable_sun", PushConstantType::BOOL, true);
    constants.set("sun_vector", PushConstantType::VEC3, _sunvector);
    constants.set("sun_colour", PushConstantType::VEC3, _sunColour.rgb());

    _updateLights = false;
    return;
}

void MainScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    if (state().playerState(idx).cameraUpdated()) {
        Camera& cam = getPlayerForIndex(idx)->getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        if (!_freeflyCamera) {
            F32 terrainHeight = 0.0f;
            vec3<F32> eyePosition = cam.getEye();

            vectorImpl<SceneGraphNode*> terrains = Object3D::filterByType(_sceneGraph->getNodesByType(SceneNodeType::TYPE_OBJECT3D), Object3D::ObjectType::TERRAIN);

            for (SceneGraphNode* terrainNode : terrains) {
                const std::shared_ptr<Terrain>& ter = terrainNode->getNode<Terrain>();
                assert(ter != nullptr);
                CLAMP<F32>(eyePosition.x,
                           ter->getDimensions().width * 0.5f * -1.0f,
                           ter->getDimensions().width * 0.5f);
                CLAMP<F32>(eyePosition.z,
                           ter->getDimensions().height * 0.5f * -1.0f,
                           ter->getDimensions().height * 0.5f);

                vec3<F32> position = terrainNode->get<TransformComponent>()->getWorldMatrix() *
                                     ter->getPositionFromGlobal(eyePosition.x, eyePosition.z);
                terrainHeight = position.y;
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

    Scene::processInput(idx, deltaTimeUS);
}

void MainScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.5);
    D64 TimeDisplay = Time::SecondsToMilliseconds(1.0);

    if (_guiTimersMS[0] >= FpsDisplay) {
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
        _guiTimersMS[0] = 0.0;
    }

    if (_guiTimersMS[1] >= TimeDisplay) {
        _GUI->modifyText(_ID("timeDisplay"),
                         Util::StringFormat("Elapsed time: %5.0f", Time::ElapsedSeconds()));
        _guiTimersMS[1] = 0.0;
    }

    Scene::processGUI(deltaTimeUS);
}

void MainScene::processTasks(const U64 deltaTimeUS) {
    updateLights();
    D64 SunDisplay = Time::SecondsToMilliseconds(1.50);
    if (_taskTimers[0] >= SunDisplay) {
        _sunAngle.y += 0.0005f;
        _sunvector = vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                               -cosf(_sunAngle.y),
                               -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);
        _taskTimers[0] = 0.0;
        _updateLights = true;


        vectorImpl<SceneGraphNode*> terrains = Object3D::filterByType(_sceneGraph->getNodesByType(SceneNodeType::TYPE_OBJECT3D), Object3D::ObjectType::TERRAIN);

        //for (SceneGraphNode* terrainNode : terrains) {
            //terrainNode.lock()->get<TransformComponent>()->setPositionY(terrainNode.lock()->get<TransformComponent>()->getPosition().y - 0.5f);
        //}
    }

    Scene::processTasks(deltaTimeUS);
}

bool MainScene::load(const stringImpl& name) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    Camera* baseCamera = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT);
    baseCamera->setMoveSpeedFactor(10.0f);

    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _sun->getNode<DirectionalLight>()->csmSplitCount(3);  // 3 splits
    _sun->getNode<DirectionalLight>()->csmSplitLogFactor(0.965f);
    _sun->getNode<DirectionalLight>()->csmNearClipOffset(25.0f);
    _currentSky = addSky();

    static const U32 normalMask = to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::TRANSFORM) |
                                  to_base(ComponentType::BOUNDS) |
                                  to_base(ComponentType::RENDERING) |
                                  to_base(ComponentType::NAVIGATION);

    g_waterDimensions.x = baseCamera->getZPlanes().y;
    g_waterDimensions.z = baseCamera->getZPlanes().y;
    ResourceDescriptor infiniteWater("waterEntity");
    infiniteWater.setUserPtr(g_waterDimensions);
    WaterPlane_ptr water = CreateResource<WaterPlane>(_resCache, infiniteWater);
    SceneGraphNode* waterGraphNode = _sceneGraph->getRoot().addNode(water, normalMask, PhysicsGroup::GROUP_IGNORE);
    waterGraphNode->usageContext(SceneGraphNode::UsageContext::NODE_STATIC);
    waterGraphNode->get<NavigationComponent>()->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);
    waterGraphNode->get<TransformComponent>()->setPositionY(state().waterLevel());
    PushConstants& constants = waterGraphNode->get<RenderingComponent>()->pushConstants();
    constants.set("_waterShininess", PushConstantType::FLOAT, 50.0f);
    constants.set("_noiseFactor", PushConstantType::VEC2, vec2<F32>(10.0f, 10.0f));
    constants.set("_noiseTile", PushConstantType::VEC2, vec2<F32>(0.1f, 0.1f));
    return loadState;
}

U16 MainScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {_context.sfx().playSound(_beep);});
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
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
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_M, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _freeflyCamera = !_freeflyCamera;
        Camera& cam = _scenePlayers[getPlayerIndexForDevice(param._deviceIndex)]->getCamera();
        cam.setMoveSpeedFactor(_freeflyCamera ? 20.0f : 10.0f);
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        vectorImpl<SceneGraphNode*> terrains = Object3D::filterByType(_sceneGraph->getNodesByType(SceneNodeType::TYPE_OBJECT3D), Object3D::ObjectType::TERRAIN);

        for (SceneGraphNode* terrainNode : terrains) {
            terrainNode->getNode<Terrain>()->toggleBoundingBoxes();
        }
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_T, actions);

    return actionID++;
}

bool MainScene::unload() {
    _context.sfx().stopMusic();
    _context.sfx().stopAllSounds();
    g_boxMoveTaskID = 0;

    return Scene::unload();
}

void MainScene::test(const Task& parentTask, AnyParam a, CallbackParam b) {
    if(!parentTask.stopRequested()) {
        static bool switchAB = false;
        vec3<F32> pos;
        SceneGraphNode* boxNode(_sceneGraph->findNode("box"));

        std::shared_ptr<Object3D> box;
        if (boxNode) {
            box = boxNode->getNode<Object3D>();
        }
        if (box) {
            pos = boxNode->get<TransformComponent>()->getPosition();
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
        if (box) boxNode->get<TransformComponent>()->setPosition(pos);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        if (g_boxMoveTaskID != 0) {
            if (!parentTask.stopRequested()) {
                g_boxMoveTaskID = registerTask(CreateTask(platformContext(), 
                                               getGUID(),
                                               DELEGATE_BIND(&MainScene::test,
                                                             this,
                                                             std::placeholders::_1,
                                                             stringImpl("test"),
                                                             CallbackParam::TYPE_STRING)));
            }
        }
    }
}

bool MainScene::loadResources(bool continueOnErrors) {
    _taskTimers.push_back(0.0); // Sun
    _guiTimersMS.push_back(0.0);  // Fps
    _guiTimersMS.push_back(0.0);  // Time

    _sunAngle = vec2<F32>(0.0f, Angle::to_RADIANS(45.0f));
    _sunvector =
        vec4<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y), 0.0f);

    removeTask(g_boxMoveTaskID);
    g_boxMoveTaskID = registerTask(CreateTask(platformContext(),
                                              getGUID(),
                                              DELEGATE_BIND(&MainScene::test,
                                              this,
                                              std::placeholders::_1,
                                              stringImpl("test"),
                                              CallbackParam::TYPE_STRING)));

    ResourceDescriptor beepSound("beep sound");
    beepSound.setResourceName("beep.wav");
    beepSound.setResourceLocation(Paths::g_assetsLocation + Paths::g_soundsLocation);
    beepSound.setFlag(false);
    _beep = CreateResource<AudioDescriptor>(_resCache, beepSound);

    return true;
}

void MainScene::postLoadMainThread() {
    _GUI->addText("fpsDisplay",  // Unique ID
        pixelPosition(60, 60),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec3<F32>(0.0f, 0.2f, 1.0f),  // Colour
        Util::StringFormat("FPS: %d", 0));  // Text and arguments

    _GUI->addText("timeDisplay", pixelPosition(60, 80), Font::DIVIDE_DEFAULT,
        UColour(164, 64, 64, 255),
        Util::StringFormat("Elapsed time: %5.0f", Time::ElapsedSeconds()));
    _GUI->addText("underwater", pixelPosition(60, 115), Font::DIVIDE_DEFAULT,
                  UColour(64, 200, 64, 255),
        Util::StringFormat("Underwater [ %s ] | WaterLevel [%f] ]", "false", 0));
    _GUI->addText("RenderBinCount", pixelPosition(60, 135), Font::BATANG,
                  UColour(164, 64, 64, 255),
        Util::StringFormat("Number of items in Render Bin: %d", 0));

    const vec3<F32>& eyePos = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->getEye();
    const vec3<F32>& euler = Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->getEuler();
    _GUI->addText("camPosition", pixelPosition(60, 100), Font::DIVIDE_DEFAULT,
                  UColour(64, 200, 64, 255),
        Util::StringFormat("Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
            eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));

    Scene::postLoadMainThread();
}

};

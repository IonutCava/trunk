#include "stdafx.h"

#include "Headers/TenisScene.h"
#include "Headers/TenisSceneAIProcessor.h"

#include "AI/ActionInterface/Headers/AITeam.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Headers/PlatformContext.h"
#include "GUI/Headers/GUIButton.h"
#include "GUI/Headers/GUI.h"

namespace Divide {

namespace {
    I64 g_gameTaskID = 0;
    std::atomic_bool s_gameStarted;
};

TenisScene::TenisScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Scene(context, cache, parent, name),
    _ball(nullptr)
{
    _sideImpulseFactor = 0;
    _directionTeam1ToTeam2 = true;
    _upwardsDirection = true;
    _touchedTerrainTeam1 = false;
    _touchedTerrainTeam2 = false;
    _lostTeam1 = false;
    _applySideImpulse = false;
    _scoreTeam1 = 0;
    _scoreTeam2 = 0;
    _gamePlaying = false;
    _team1 = nullptr;
    _team2 = nullptr;
}

void TenisScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = 0.7;
    if (_guiTimersMS[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _GUI->modifyText(_ID("RenderBinCount"),
                         Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                         _context.gfx().parent().renderPassManager().getLastTotalBinSize(RenderStage::DISPLAY),
                         _context.gfx().getLastCullCount()));
        _guiTimersMS[0] = 0.0;
    }
    Scene::processGUI(deltaTimeUS);
}

void TenisScene::processTasks(const U64 deltaTimeUS) {
    Scene::processTasks(deltaTimeUS);

    checkCollisions();

    if (s_gameStarted && !_gamePlaying && _scoreTeam1 < 10 && _scoreTeam2 < 10)
        startGame(-1);

    if (_scoreTeam1 == 10 || _scoreTeam2 == 10) {
        s_gameStarted = false;
        _GUI->modifyText(_ID("Message"), Util::StringFormat("Team %d won!", _scoreTeam1 == 10 ? 1 : 2));
    }

    vec2<F32> _sunAngle = vec2<F32>(0.0f, Angle::to_RADIANS(45.0f));
    _sunvector =
        vec3<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
            -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    _sun->get<TransformComponent>()->setPosition(_sunvector);

    PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
    constants.set("enable_sun", GFX::PushConstantType::BOOL, true);
    constants.set("sun_vector", GFX::PushConstantType::VEC3, _sunvector);
    constants.set("sun_colour", GFX::PushConstantType::VEC3, _sun->getNode<Light>()->getDiffuseColour());
}

void TenisScene::resetGame() {
    removeTask(g_gameTaskID);
    _collisionPlayer1 = false;
    _collisionPlayer2 = false;
    _collisionPlayer3 = false;
    _collisionPlayer4 = false;
    _collisionNet = false;
    _collisionFloor = false;
    _gamePlaying = true;
    _upwardsDirection = true;
    _touchedTerrainTeam1 = false;
    _touchedTerrainTeam2 = false;
    _applySideImpulse = false;
    _sideImpulseFactor = 0;
    WriteLock w_lock(_gameLock);
    _ballSGN->get<TransformComponent>()->setPosition(vec3<F32>(
        (Random(0, 10) >= 5) ? 3.0f : -3.0f, 0.2f, _lostTeam1 ? -7.0f : 7.0f));
    _directionTeam1ToTeam2 = !_lostTeam1;
    _lostTeam1 = false;
    s_gameStarted = true;
}

void TenisScene::startGame(I64 btnGUID) {
    resetGame();

    TaskHandle newGame(CreateTask(platformContext(),
                                  getGUID(),
                               DELEGATE_BIND(&TenisScene::playGame, this,
                                             std::placeholders::_1,
                                             Random(4),
                                             CallbackParam::TYPE_INTEGER)));
    g_gameTaskID = registerTask(newGame);
}

void TenisScene::checkCollisions() {
    BoundingBox ballBB = _ballSGN->get<BoundsComponent>()->getBoundingBox();
    BoundingBox floorBB = _floor->get<BoundsComponent>()->getBoundingBox();

    WriteLock w_lock(_gameLock);
    _collisionPlayer1 = ballBB.collision(_aiPlayer[0]->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer2 = ballBB.collision(_aiPlayer[1]->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer3 = ballBB.collision(_aiPlayer[2]->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer4 = ballBB.collision(_aiPlayer[3]->get<BoundsComponent>()->getBoundingBox());
    _collisionNet = ballBB.collision(_net->get<BoundsComponent>()->getBoundingBox());
    _collisionFloor = floorBB.collision(ballBB);
}

// Team 1: Player1 + Player2
// Team 2: Player3 + Player4
void TenisScene::playGame(const Task& parentTask, AnyParam a, CallbackParam b) {
    while (!parentTask.stopRequested()) {
        bool updated = false;
        stringImpl message;
        if (!_gamePlaying) {
            continue;
        }

        UpgradableReadLock ur_lock(_gameLock);
        // Store by copy (thread-safe) current ball position (getPosition()) should
        // be threadsafe
        vec3<F32> netPosition = _net->get<TransformComponent>()->getPosition();
        vec3<F32> ballPosition = _ballSGN->get<TransformComponent>()->getPosition();
        vec3<F32> player1Pos = _aiPlayer[0]->get<TransformComponent>()->getPosition();
        vec3<F32> player2Pos = _aiPlayer[1]->get<TransformComponent>()->getPosition();
        vec3<F32> player3Pos = _aiPlayer[2]->get<TransformComponent>()->getPosition();
        vec3<F32> player4Pos = _aiPlayer[3]->get<TransformComponent>()->getPosition();
        vec3<F32> netBBMax = _net->get<BoundsComponent>()->getBoundingBox().getMax();
        vec3<F32> netBBMin = _net->get<BoundsComponent>()->getBoundingBox().getMin();

        UpgradeToWriteLock uw_lock(ur_lock);
        // Is the ball traveling from team 1 to team 2?
        _directionTeam1ToTeam2 ? ballPosition.z -= 0.123f : ballPosition.z +=
                                                            0.123f;
        // Is the ball traveling upwards or is it falling?
        _upwardsDirection ? ballPosition.y += 0.066f : ballPosition.y -= 0.066f;
        // In case of a side drift, update accordingly
        if (_applySideImpulse) ballPosition.x += _sideImpulseFactor;

        // After we finish our computations, apply the new transform
        // setPosition/getPosition should be thread-safe
        _ballSGN->get<TransformComponent>()->setPosition(ballPosition);
        // Add a spin to the ball just for fun ...
        _ballSGN->get<TransformComponent>()->rotate(
            vec3<F32>(ballPosition.z, 1, 1));

        //----------------------COLLISIONS------------------------------//
        // z = depth. Descending to the horizon

        if (_collisionFloor) {
            if (ballPosition.z > netPosition.z) {
                _touchedTerrainTeam1 = true;
                _touchedTerrainTeam2 = false;
            } else {
                _touchedTerrainTeam1 = false;
                _touchedTerrainTeam2 = true;
            }
            _upwardsDirection = true;
        }

        // Where does the Kinetic  energy of the ball run out?
        if (ballPosition.y > 3.5) _upwardsDirection = false;

        // Did we hit a player?

        bool collisionTeam1 = _collisionPlayer1 || _collisionPlayer2;
        bool collisionTeam2 = _collisionPlayer3 || _collisionPlayer4;

        if (collisionTeam1 || collisionTeam2) {
            // Something went very wrong. Very.
            // assert(collisionTeam1 != collisionTeam2);
            if (collisionTeam1 == collisionTeam2) {
                collisionTeam1 = !_directionTeam1ToTeam2;
                collisionTeam2 = !collisionTeam1;
                Console::d_errorfn("COLLISION ERROR");
            }

            F32 sideDrift = 0;

            if (collisionTeam1) {
                _collisionPlayer1 ? sideDrift = player1Pos.x : sideDrift =
                                                                   player2Pos.x;
            } else if (collisionTeam2) {
                _collisionPlayer3 ? sideDrift = player3Pos.x : sideDrift =
                                                                   player4Pos.x;
            }

            _sideImpulseFactor = (ballPosition.x - sideDrift);
            _applySideImpulse = !IS_ZERO(_sideImpulseFactor);
            if (_applySideImpulse) _sideImpulseFactor *= 0.025f;

            _directionTeam1ToTeam2 = collisionTeam1;
        }

        //-----------------VALIDATING THE RESULTS----------------------//
        // Which team won?
        if (ballPosition.z >= player1Pos.z && ballPosition.z >= player2Pos.z) {
            _lostTeam1 = true;
            updated = true;
        }

        if (ballPosition.z <= player3Pos.z && ballPosition.z <= player4Pos.z) {
            _lostTeam1 = false;
            updated = true;
        }
        // Which team kicked the ball in the net?
        if (_collisionNet) {
            if (ballPosition.y < netBBMax.y - 0.25) {
                if (_directionTeam1ToTeam2) {
                    _lostTeam1 = true;
                } else {
                    _lostTeam1 = false;
                }
                updated = true;
            }
        }

        if (ballPosition.x + 0.5f < netBBMin.x ||
            ballPosition.x + 0.5f > netBBMax.x) {
            // If we hit the ball and it touched the opposing team's terrain
            // Or if the opposing team hit the ball but it didn't land in our
            // terrain
            if (_collisionFloor) {
                if ((_touchedTerrainTeam2 && _directionTeam1ToTeam2) ||
                    (!_directionTeam1ToTeam2 && !_touchedTerrainTeam1)) {
                    _lostTeam1 = false;
                } else {
                    _lostTeam1 = true;
                }
                updated = true;
            }
        }

        //-----------------DISPLAY RESULTS---------------------//
        if (updated) {
            if (_lostTeam1) {
                message = "Team 2 scored!";
                _scoreTeam2++;
            } else {
                message = "Team 1 scored!";
                _scoreTeam1++;
            }
            I32 score1 = _scoreTeam1;
            I32 score2 = _scoreTeam2;
            _GUI->modifyText(_ID("Team1Score"), Util::StringFormat("Team 1 Score: %d", score1));
            _GUI->modifyText(_ID("Team2Score"), Util::StringFormat("Team 2 Score: %d", score2));
            _GUI->modifyText(_ID("Message"), (char*)message.c_str());
            _gamePlaying = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

void TenisScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    Scene::processInput(idx, deltaTimeUS);
}

bool TenisScene::load(const stringImpl& name) {
    s_gameStarted = false;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);

    // Add a light
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();

    // Position camera
    // renderState().getCamera().setEye(vec3<F32>(14,5.5f,11.5f));
    // renderState().getCamera().setRotation(10/*yaw*/,-45/*pitch*/);

    //------------------------ Load up game elements
    //-----------------------------///
    _net = _sceneGraph->findNode("Net");
    //_net->forEachChild([](const SceneGraphNode& child) {
    //    child.setReceivesShadows(false);
    //});

    _floor = _sceneGraph->findNode("Floor");
    _floor->get<RenderingComponent>()->toggleRenderOption(RenderingComponent::RenderOptions::CAST_SHADOWS, false);

    _aiManager->pauseUpdate(false);
    return loadState;
}

bool TenisScene::initializeAI(bool continueOnErrors) {
    bool state = false;
    SceneGraphNode* player[4];
    //----------------------------ARTIFICIAL INTELLIGENCE------------------------------//
    AI::AIEntity* aiPlayer[4];
    for (U8 i = 0; i < 4; ++i) {
        stringImpl nodeName = Util::StringFormat("Player%d", i);
        player[i] = _sceneGraph->findNode(nodeName);
        player[i]->setSelectable(true);
        UnitComponent* unitComp = player[i]->get<UnitComponent>();

        aiPlayer[i] = MemoryManager_NEW AI::AIEntity(player[i]->get<TransformComponent>()->getPosition(), nodeName);
        aiPlayer[i]->addSensor(AI::SensorType::VISUAL_SENSOR);
        aiPlayer[i]->setAIProcessor(MemoryManager_NEW AI::TenisSceneAIProcessor(_ballSGN, *_aiManager));

        unitComp->setUnit(std::make_shared<NPC>(aiPlayer[i]));
    }

    _team1 = MemoryManager_NEW AI::AITeam(1, *_aiManager);
    _team2 = MemoryManager_NEW AI::AITeam(2, *_aiManager);

    if (state || continueOnErrors) {
        state = _aiManager->registerEntity(0, aiPlayer[0]);
    }
    if (state || continueOnErrors) {
        state = _aiManager->registerEntity(0, aiPlayer[1]);
    }
    if (state || continueOnErrors) {
        state = _aiManager->registerEntity(1, aiPlayer[2]);
    }
    if (state || continueOnErrors) {
        state = _aiManager->registerEntity(1, aiPlayer[3]);
    }
    if (state || continueOnErrors) {
        //----------------------- AI controlled units (NPC's)---------------------//
        player[0]->get<UnitComponent>()->getUnit<NPC>()->setMovementSpeed(1.45f);
        player[1]->get<UnitComponent>()->getUnit<NPC>()->setMovementSpeed(1.5f);
        player[2]->get<UnitComponent>()->getUnit<NPC>()->setMovementSpeed(1.475f);
        player[3]->get<UnitComponent>()->getUnit<NPC>()->setMovementSpeed(1.45f);
        Scene::initializeAI(continueOnErrors);
    }

    return state;
}

bool TenisScene::deinitializeAI(bool continueOnErrors) {
    _aiManager->pauseUpdate(true);
    for (U8 i = 0; i < 4; ++i) {
        _aiManager->unregisterEntity(_aiPlayer[i]->get<UnitComponent>()->getUnit<NPC>()->getAIEntity());
    }
    MemoryManager::DELETE(_team1);
    MemoryManager::DELETE(_team2);
    return Scene::deinitializeAI(continueOnErrors);
}

bool TenisScene::loadResources(bool continueOnErrors) {
    static const U32 normalMask = to_base(ComponentType::TRANSFORM) |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::BOUNDS) |
                                  to_base(ComponentType::RENDERING) |
                                  to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::NETWORKING);

    // Create our ball
    ResourceDescriptor ballDescriptor("Tenis Ball");
    _ball = CreateResource<Sphere3D>(_resCache, ballDescriptor);
    _ball->getMaterialTpl()->setDiffuse(FColour(0.4f, 0.5f, 0.5f, 1.0f));
    _ball->getMaterialTpl()->setShininess(0.2f);
    _ball->getMaterialTpl()->setSpecular(FColour(0.7f, 0.7f, 0.7f, 1.0f));
    _ball->setResolution(16);
    _ball->setRadius(0.3f);
    _ballSGN = _sceneGraph->getRoot().addNode(_ball, normalMask, PhysicsGroup::GROUP_KINEMATIC, "TenisBallSGN");
    _ballSGN->get<TransformComponent>()->translate(vec3<F32>(3.0f, 0.2f, 7.0f));
    _ballSGN->setSelectable(true);


    _guiTimersMS.push_back(0.0);  // Fps
    return true;
}

void TenisScene::postLoadMainThread() {
    const vec2<U16>& resolution = _context.gfx().renderingResolution();

    GUIButton* btn = _GUI->addButton(_ID("Serve"),
                                      "Serve",
                                      pixelPosition(resolution.width - 220, 60),
                                      pixelScale(100, 25));
    btn->setTooltip("Start a new game!");
    btn->setEventCallback(GUIButton::Event::MouseClick,
                          DELEGATE_BIND(&TenisScene::startGame, this, std::placeholders::_1));
    _GUI->addText(
        "Team1Score", pixelPosition(to_I32(resolution.width - 250),
            to_I32(resolution.height / 1.3f)),
        Font::DIVIDE_DEFAULT,
        UColour(0, 192, 192, 255),
        Util::StringFormat("Team 1 Score: %d", 0));

    _GUI->addText(
        "Team2Score", pixelPosition(to_I32(resolution.width - 250),
            to_I32(resolution.height / 1.5f)),
        Font::DIVIDE_DEFAULT,
        UColour(50, 192, 0, 255),
        Util::StringFormat("Team 2 Score: %d", 0));

    _GUI->addText("Message",
                  pixelPosition(to_I32(resolution.width - 250),
            to_I32(resolution.height / 1.7f)),
        Font::DIVIDE_DEFAULT,
                  UColour(0, 255, 0, 255),
        "");

    _GUI->addText("fpsDisplay",  // Unique ID
                  pixelPosition(60, 60),  // Position
        Font::DIVIDE_DEFAULT,  // Font
                  UColour(0, 50, 255, 255),// Colour
        Util::StringFormat("FPS: %d", 0));  // Text and arguments

    _GUI->addText("RenderBinCount", pixelPosition(60, 70), Font::DIVIDE_DEFAULT,
                  UColour(164, 50, 50, 255),
        Util::StringFormat("Number of items in Render Bin: %d", 0));

    Scene::postLoadMainThread();
}

};

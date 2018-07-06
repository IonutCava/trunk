#include "Headers/TenisScene.h"
#include "Headers/TenisSceneAIProcessor.h"

#include "AI/ActionInterface/Headers/AITeam.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Geometry/Material/Headers/Material.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Math/Headers/Transform.h"
#include "Managers/Headers/AIManager.h"
#include "GUI/Headers/GUIButton.h"
#include "GUI/Headers/GUI.h"

namespace Divide {

namespace {
    std::atomic_bool s_gameStarted;
};


void TenisScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = 0.7;
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _GUI->modifyText(_ID("RenderBinCount"),
                         Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                                            GFX_RENDER_BIN_SIZE, GFX_HIZ_CULL_COUNT));
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void TenisScene::processTasks(const U64 deltaTime) {
    Scene::processTasks(deltaTime);

    checkCollisions();

    if (s_gameStarted && !_gamePlaying && _scoreTeam1 < 10 && _scoreTeam2 < 10)
        startGame(-1);

    if (_scoreTeam1 == 10 || _scoreTeam2 == 10) {
        s_gameStarted = false;
        _GUI->modifyText(_ID("Message"), Util::StringFormat("Team %d won!", _scoreTeam1 == 10 ? 1 : 2));
    }

    vec2<F32> _sunAngle = vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec3<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
            -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    _sun.lock()->get<PhysicsComponent>()->setPosition(_sunvector);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(_sunvector,
        _sun.lock()->getNode<Light>()->getDiffuseColor());
}

void TenisScene::resetGame() {
    removeTask(getGUID());
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
    _ballSGN.lock()->get<PhysicsComponent>()->setPosition(vec3<F32>(
        (Random(0, 10) >= 5) ? 3.0f : -3.0f, 0.2f, _lostTeam1 ? -7.0f : 7.0f));
    _directionTeam1ToTeam2 = !_lostTeam1;
    _lostTeam1 = false;
    s_gameStarted = true;
}

void TenisScene::startGame(I64 btnGUID) {
    resetGame();

    TaskHandle newGame(CreateTask(getGUID(),
                               DELEGATE_BIND(&TenisScene::playGame, this,
                                             std::placeholders::_1,
                                             rand() % 5,
                                             CallbackParam::TYPE_INTEGER)));
    newGame.startTask(Task::TaskPriority::HIGH);
    registerTask(newGame);
}

void TenisScene::checkCollisions() {
    SceneGraphNode_ptr Player1(_aiPlayer1->getUnitRef()->getBoundNode().lock());
    SceneGraphNode_ptr Player2(_aiPlayer2->getUnitRef()->getBoundNode().lock());
    SceneGraphNode_ptr Player3(_aiPlayer3->getUnitRef()->getBoundNode().lock());
    SceneGraphNode_ptr Player4(_aiPlayer4->getUnitRef()->getBoundNode().lock());

    BoundingBox ballBB = _ballSGN.lock()->get<BoundsComponent>()->getBoundingBox();
    BoundingBox floorBB = _floor.lock()->get<BoundsComponent>()->getBoundingBox();

    WriteLock w_lock(_gameLock);
    _collisionPlayer1 = ballBB.collision(Player1->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer2 = ballBB.collision(Player2->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer3 = ballBB.collision(Player3->get<BoundsComponent>()->getBoundingBox());
    _collisionPlayer4 = ballBB.collision(Player4->get<BoundsComponent>()->getBoundingBox());
    _collisionNet = ballBB.collision(_net.lock()->get<BoundsComponent>()->getBoundingBox());
    _collisionFloor = floorBB.collision(ballBB);
}

// Team 1: Player1 + Player2
// Team 2: Player3 + Player4
void TenisScene::playGame(const std::atomic_bool& stopRequested, cdiggins::any a, CallbackParam b) {
    while (!stopRequested) {
        bool updated = false;
        stringImpl message;
        if (!_gamePlaying) {
            continue;
        }

        UpgradableReadLock ur_lock(_gameLock);
        // Shortcut to the scene graph nodes containing our agents
        SceneGraphNode_ptr Player1(_aiPlayer1->getUnitRef()->getBoundNode().lock());
        SceneGraphNode_ptr Player2(_aiPlayer2->getUnitRef()->getBoundNode().lock());
        SceneGraphNode_ptr Player3(_aiPlayer3->getUnitRef()->getBoundNode().lock());
        SceneGraphNode_ptr Player4(_aiPlayer4->getUnitRef()->getBoundNode().lock());

        // Store by copy (thread-safe) current ball position (getPosition()) should
        // be threadsafe
        vec3<F32> netPosition =
            _net.lock()->get<PhysicsComponent>()->getPosition();
        vec3<F32> ballPosition =
            _ballSGN.lock()->get<PhysicsComponent>()->getPosition();
        vec3<F32> player1Pos =
            Player1->get<PhysicsComponent>()->getPosition();
        vec3<F32> player2Pos =
            Player2->get<PhysicsComponent>()->getPosition();
        vec3<F32> player3Pos =
            Player3->get<PhysicsComponent>()->getPosition();
        vec3<F32> player4Pos =
            Player4->get<PhysicsComponent>()->getPosition();
        vec3<F32> netBBMax = _net.lock()->get<BoundsComponent>()->getBoundingBox().getMax();
        vec3<F32> netBBMin = _net.lock()->get<BoundsComponent>()->getBoundingBox().getMin();

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
        _ballSGN.lock()->get<PhysicsComponent>()->setPosition(ballPosition);
        // Add a spin to the ball just for fun ...
        _ballSGN.lock()->get<PhysicsComponent>()->rotate(
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

void TenisScene::processInput(const U64 deltaTime) {}

bool TenisScene::load(const stringImpl& name, GUI* const gui) {
    s_gameStarted = false;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);

    // Add a light
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();

    // Position camera
    // renderState().getCamera().setEye(vec3<F32>(14,5.5f,11.5f));
    // renderState().getCamera().setRotation(10/*yaw*/,-45/*pitch*/);

    //------------------------ Load up game elements
    //-----------------------------///
    _net = _sceneGraph->findNode("Net");
    //U32 childCount = _net->getChildCount();
    //for (U32 i = 0; i < childCount; ++i) {
    //    SceneGraphNode& child = _net->getChild(i);
    //    child.setReceivesShadows(false);
    //}
    _floor = _sceneGraph->findNode("Floor");
    _floor.lock()->get<RenderingComponent>()->castsShadows(false);

    AI::AIManager::instance().pauseUpdate(false);
    return loadState;
}

bool TenisScene::initializeAI(bool continueOnErrors) {
    bool state = false;
    SceneGraphNode_ptr player[4];
    //----------------------------ARTIFICIAL
    //INTELLIGENCE------------------------------//
    player[0] = _sceneGraph->findNode("Player1").lock();
    player[1] = _sceneGraph->findNode("Player2").lock();
    player[2] = _sceneGraph->findNode("Player3").lock();
    player[3] = _sceneGraph->findNode("Player4").lock();
    player[0]->setSelectable(true);
    player[1]->setSelectable(true);
    player[2]->setSelectable(true);
    player[3]->setSelectable(true);

    _aiPlayer1 = MemoryManager_NEW AI::AIEntity(
        player[0]->get<PhysicsComponent>()->getPosition(), "Player1");
    _aiPlayer2 = MemoryManager_NEW AI::AIEntity(
        player[1]->get<PhysicsComponent>()->getPosition(), "Player2");
    _aiPlayer3 = MemoryManager_NEW AI::AIEntity(
        player[2]->get<PhysicsComponent>()->getPosition(), "Player3");
    _aiPlayer4 = MemoryManager_NEW AI::AIEntity(
        player[3]->get<PhysicsComponent>()->getPosition(), "Player4");
    _aiPlayer1->addSensor(AI::SensorType::VISUAL_SENSOR);
    _aiPlayer2->addSensor(AI::SensorType::VISUAL_SENSOR);
    _aiPlayer3->addSensor(AI::SensorType::VISUAL_SENSOR);
    _aiPlayer4->addSensor(AI::SensorType::VISUAL_SENSOR);

    _aiPlayer1->setAIProcessor(
        MemoryManager_NEW AI::TenisSceneAIProcessor(_ballSGN));
    _aiPlayer2->setAIProcessor(
        MemoryManager_NEW AI::TenisSceneAIProcessor(_ballSGN));
    _aiPlayer3->setAIProcessor(
        MemoryManager_NEW AI::TenisSceneAIProcessor(_ballSGN));
    _aiPlayer4->setAIProcessor(
        MemoryManager_NEW AI::TenisSceneAIProcessor(_ballSGN));

    _team1 = MemoryManager_NEW AI::AITeam(1);
    _team2 = MemoryManager_NEW AI::AITeam(2);

    if (state || continueOnErrors) {
        state = AI::AIManager::instance().registerEntity(0, _aiPlayer1);
    }
    if (state || continueOnErrors) {
        state = AI::AIManager::instance().registerEntity(0, _aiPlayer2);
    }
    if (state || continueOnErrors) {
        state = AI::AIManager::instance().registerEntity(1, _aiPlayer3);
    }
    if (state || continueOnErrors) {
        state = AI::AIManager::instance().registerEntity(1, _aiPlayer4);
    }
    if (state || continueOnErrors) {
        //----------------------- AI controlled units (NPC's)
        //---------------------//
        _player1 = MemoryManager_NEW NPC(player[0], _aiPlayer1);
        _player2 = MemoryManager_NEW NPC(player[1], _aiPlayer2);
        _player3 = MemoryManager_NEW NPC(player[2], _aiPlayer3);
        _player4 = MemoryManager_NEW NPC(player[3], _aiPlayer4);

        _player1->setMovementSpeed(1.45f);
        _player2->setMovementSpeed(1.5f);
        _player3->setMovementSpeed(1.475f);
        _player4->setMovementSpeed(1.45f);
    }

    if (state || continueOnErrors) Scene::initializeAI(continueOnErrors);
    return state;
}

bool TenisScene::deinitializeAI(bool continueOnErrors) {
    AI::AIManager::instance().pauseUpdate(true);

    AI::AIManager::instance().unregisterEntity(_aiPlayer1);
    AI::AIManager::instance().unregisterEntity(_aiPlayer2);
    AI::AIManager::instance().unregisterEntity(_aiPlayer3);
    AI::AIManager::instance().unregisterEntity(_aiPlayer4);
    MemoryManager::DELETE(_aiPlayer1);
    MemoryManager::DELETE(_aiPlayer2);
    MemoryManager::DELETE(_aiPlayer3);
    MemoryManager::DELETE(_aiPlayer4);
    MemoryManager::DELETE(_player1);
    MemoryManager::DELETE(_player2);
    MemoryManager::DELETE(_player3);
    MemoryManager::DELETE(_player4);
    MemoryManager::DELETE(_team1);
    MemoryManager::DELETE(_team2);
    return Scene::deinitializeAI(continueOnErrors);
}

bool TenisScene::loadResources(bool continueOnErrors) {
    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING) |
                                  to_const_uint(SGNComponent::ComponentType::NAVIGATION);

    // Create our ball
    ResourceDescriptor ballDescriptor("Tenis Ball");
    _ball = CreateResource<Sphere3D>(ballDescriptor);
    _ball->getMaterialTpl()->setDiffuse(vec4<F32>(0.4f, 0.5f, 0.5f, 1.0f));
    _ball->getMaterialTpl()->setShininess(0.2f);
    _ball->getMaterialTpl()->setSpecular(vec4<F32>(0.7f, 0.7f, 0.7f, 1.0f));
    _ball->setResolution(16);
    _ball->setRadius(0.3f);
    _ballSGN = _sceneGraph->getRoot().addNode(*_ball, normalMask, PhysicsGroup::GROUP_KINEMATIC, "TenisBallSGN");
    _ballSGN.lock()->get<PhysicsComponent>()->translate(
        vec3<F32>(3.0f, 0.2f, 7.0f));
    _ballSGN.lock()->setSelectable(true);

    const vec2<U16>& resolution = _GUI->getDisplayResolution();

    GUIElement* btn = _GUI->addButton(
        _ID("Serve"), "Serve",
        vec2<I32>(resolution.width - 220, 60),
        vec2<U32>(100, 25), vec3<F32>(0.65f, 0.65f, 0.65f),
        DELEGATE_BIND(&TenisScene::startGame, this, std::placeholders::_1));
    btn->setTooltip("Start a new game!");

    _GUI->addText(
        _ID("Team1Score"), vec2<I32>(to_int(resolution.width - 250),
                                to_int(resolution.height / 1.3f)),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(0, 192, 192, 255),
        Util::StringFormat("Team 1 Score: %d", 0));

    _GUI->addText(
        _ID("Team2Score"), vec2<I32>(to_int(resolution.width - 250),
                                to_int(resolution.height / 1.5f)),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(50, 192, 0, 255),
        Util::StringFormat("Team 2 Score: %d", 0));

    _GUI->addText(_ID("Message"),
                  vec2<I32>(to_int(resolution.width - 250),
                            to_int(resolution.height / 1.7f)),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(0, 255, 0, 255),
                  "");

    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec4<U8>(0, 50, 255, 255),// Color
                  Util::StringFormat("FPS: %d", 0));  // Text and arguments

    _GUI->addText(_ID("RenderBinCount"), vec2<I32>(60, 70), Font::DIVIDE_DEFAULT,
                  vec4<U8>(164, 50, 50, 255),
                  Util::StringFormat("Number of items in Render Bin: %d", 0));
    _guiTimers.push_back(0.0);  // Fps
    return true;
}

};

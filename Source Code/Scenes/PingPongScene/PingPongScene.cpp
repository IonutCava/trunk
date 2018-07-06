#include "Headers/PingPongScene.h"

#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

void PingPongScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void PingPongScene::processTasks(const U64 deltaTime) {
    static vec2<F32> _sunAngle =
        vec2<F32>(0.0f, Angle::DegreesToRadians(45.0f));
    static bool direction = false;
    if (!direction) {
        _sunAngle.y += 0.005f;
        _sunAngle.x += 0.005f;
    } else {
        _sunAngle.y -= 0.005f;
        _sunAngle.x -= 0.005f;
    }

    if (_sunAngle.y <= Angle::DegreesToRadians(25) ||
        _sunAngle.y >= Angle::DegreesToRadians(70))
        direction = !direction;

    _sunvector =
        vec3<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    _sun.lock()->get<PhysicsComponent>()->setPosition(_sunvector);
    _currentSky.lock()->getNode<Sky>()->setSunProperties(_sunvector,
                                                         _sun.lock()->getNode<Light>()->getDiffuseColor());

    Scene::processTasks(deltaTime);
}

void PingPongScene::resetGame() {
    _directionTowardsAdversary = true;
    _upwardsDirection = false;
    _touchedAdversaryTableHalf = false;
    _touchedOwnTableHalf = false;
    _lost = false;
    _sideDrift = 0;
    clearTasks();
    _ballSGN.lock()->get<PhysicsComponent>()->setPosition(vec3<F32>(0, 2, 2));
}

void PingPongScene::serveBall(I64 btnGUID) {
    _GUI->modifyText(_ID("insults"), "");
    resetGame();

    removeTask(getGUID());

    TaskHandle newGame(CreateTask(getGUID(),
                               DELEGATE_BIND(&PingPongScene::test, this,
                               std::placeholders::_1,
                               rand() % 5,
                               CallbackParam::TYPE_INTEGER)));
    newGame.startTask(Task::TaskPriority::HIGH);
    registerTask(newGame);
}

void PingPongScene::test(const std::atomic_bool& stopRequested, cdiggins::any a, CallbackParam b) {
    while (!stopRequested) {
        bool updated = false;
        stringImpl message;
        PhysicsComponent* ballTransform =
            _ballSGN.lock()->get<PhysicsComponent>();
        vec3<F32> ballPosition = ballTransform->getPosition();

        SceneGraphNode_ptr table(_sceneGraph->findNode("table").lock());
        SceneGraphNode_ptr net(_sceneGraph->findNode("net").lock());
        SceneGraphNode_ptr opponent(_sceneGraph->findNode("opponent").lock());
        SceneGraphNode_ptr paddle(_sceneGraph->findNode("paddle").lock());

        vec3<F32> paddlePosition =
            paddle->get<PhysicsComponent>()->getPosition();
        vec3<F32> opponentPosition =
            opponent->get<PhysicsComponent>()->getPosition();
        vec3<F32> tablePosition =
            table->get<PhysicsComponent>()->getPosition();

        // Is the ball coming towards us or towards the opponent?
        _directionTowardsAdversary ? ballPosition.z -= 0.11f : ballPosition.z +=
                                                               0.11f;
        // Up or down?
        _upwardsDirection ? ballPosition.y += 0.084f : ballPosition.y -= 0.084f;

        // Is the ball moving to the right or to the left?
        ballPosition.x += _sideDrift * 0.15f;
        if (opponentPosition.x != ballPosition.x)
            opponent->get<PhysicsComponent>()->translateX(
                ballPosition.x - opponentPosition.x);

        ballTransform->translate(ballPosition - ballTransform->getPosition());

        // Did we hit the table? Bounce then ...
        if (table->get<BoundsComponent>()
                 ->getBoundingBox().collision(_ballSGN.lock()->get<BoundsComponent>()
                                                             ->getBoundingBox()))
        {
            if (ballPosition.z > tablePosition.z) {
                _touchedOwnTableHalf = true;
                _touchedAdversaryTableHalf = false;
            } else {
                _touchedOwnTableHalf = false;
                _touchedAdversaryTableHalf = true;
            }
            _upwardsDirection = true;
        }
        // Kinetic  energy depletion
        if (ballPosition.y > 2.1f) _upwardsDirection = false;

        // Did we hit the paddle?
        if (_ballSGN.lock()->get<BoundsComponent>()->getBoundingBox().collision(paddle->get<BoundsComponent>()->getBoundingBox())) {
            _sideDrift = ballPosition.x - paddlePosition.x;
            // If we hit the ball with the upper margin of the paddle, add a slight
            // impuls to the ball
            if (ballPosition.y >= paddlePosition.y) ballPosition.z -= 0.12f;

            _directionTowardsAdversary = true;
        }

        if (ballPosition.y + 0.75f < table->get<BoundsComponent>()->getBoundingBox().getMax().y) {
            // If we hit the ball and it landed on the opponent's table half
            // Or if the opponent hit the ball and it landed on our table half
            if ((_touchedAdversaryTableHalf && _directionTowardsAdversary) ||
                (!_directionTowardsAdversary && !_touchedOwnTableHalf))
                _lost = false;
            else
                _lost = true;

            updated = true;
        }
        // Did we win or lose?
        if (ballPosition.z >= paddlePosition.z) {
            _lost = true;
            updated = true;
        }
        if (ballPosition.z <= opponentPosition.z) {
            _lost = false;
            updated = true;
        }

        if (_ballSGN.lock()->get<BoundsComponent>()->getBoundingBox().collision(net->get<BoundsComponent>()->getBoundingBox())) {
            if (_directionTowardsAdversary) {
                // Did we hit the net?
                _lost = true;
            } else {
                // Did the opponent hit the net?
                _lost = false;
            }
            updated = true;
        }

        // Did we hit the opponent? Then change ball direction ... BUT ...
        // Add a small chance that we win
        if (Random(30) != 2)
            if (_ballSGN.lock()->get<BoundsComponent>()
                               ->getBoundingBox().collision(opponent->get<BoundsComponent>()
                                                                    ->getBoundingBox())) {
                _sideDrift =
                    ballPosition.x -
                    opponent->get<PhysicsComponent>()->getPosition().x;
                _directionTowardsAdversary = false;
            }
        // Add a spin effect to the ball
        ballTransform->rotate(vec3<F32>(ballPosition.z, 1, 1));

        if (updated) {
            if (_lost) {
                message = "You lost!";
                _score--;

                if (b == CallbackParam::TYPE_INTEGER) {
                    I32 quote = a.constant_cast<I32>();
                    if (_score % 3 == 0)
                        _GUI->modifyText(_ID("insults"), _quotes[quote]);
                }
            } else {
                message = "You won!";
                _score++;
            }

            _GUI->modifyText(_ID("Score"), Util::StringFormat("Score: %d", _score));
            _GUI->modifyText(_ID("Message"), (char*)message.c_str());
            resetGame();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void PingPongScene::processInput(const U64 deltaTime) {
    if (_freeFly) {
        _wasInFreeFly = true;
        return;
    }
    if (_wasInFreeFly) {
        // Position the camera
        _paddleCam->setPitch(-90);
        _paddleCam->setYaw(0);
        _paddleCam->setRoll(0);
        _wasInFreeFly = false;
    }
    // Move FB = Forward/Back = up/down
    // Move LR = Left/Right
    static F32 paddleMovementDivisor = 10;
    // Camera controls
    if (state().angleLR() != SceneState::MoveDirection::NONE) {
        _paddleCam->rotateYaw(to_float(state().angleLR()));
    }
    if (state().angleUD() != SceneState::MoveDirection::NONE) {
        _paddleCam->rotatePitch(to_float(state().angleUD()));
    }

    SceneGraphNode_ptr paddle(_sceneGraph->findNode("paddle").lock());

    vec3<F32> pos = paddle->get<PhysicsComponent>()->getPosition();

    // Paddle movement is limited to the [-3,3] range except for Y-descent
    if (state().moveFB() != SceneState::MoveDirection::NONE) {
        if ((state().moveFB() == SceneState::MoveDirection::POSITIVE && pos.y >= 3) ||
            (state().moveFB() == SceneState::MoveDirection::NEGATIVE && pos.y <= 0.5f))
            return;
        paddle->get<PhysicsComponent>()->translateY(to_int(state().moveFB()) / paddleMovementDivisor);
    }

    if (state().moveLR() != SceneState::MoveDirection::NONE) {
        // Left/right movement is flipped for proper control
        if ((state().moveLR() == SceneState::MoveDirection::NEGATIVE && pos.x >= 3) ||
            (state().moveLR() == SceneState::MoveDirection::POSITIVE && pos.x <= -3))
            return;
        paddle->get<PhysicsComponent>()->translateX(to_int(state().moveLR()) / paddleMovementDivisor);
    }
}

bool PingPongScene::load(const stringImpl& name, GUI* const gui) {
    _freeFly = false;
    _wasInFreeFly = false;

    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();
    _freeFlyCam = &renderState().getCamera();
    _paddleCam = renderState().getCameraMgr().createCamera("paddleCam", Camera::CameraType::FREE_FLY);
    _paddleCam->fromCamera(*_freeFlyCam);
    // Position the camera
    // renderState().getCamera().setPitch(-90);
    _paddleCam->lockMovement(true);
    _paddleCam->setEye(vec3<F32>(0, 2.5f, 6.5f));
    _freeFlyCam->setEye(vec3<F32>(0, 2.5f, 6.5f));

    return loadState;
}

U16 PingPongScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {serveBall(-1);});
    actions._onReleaseAction = actionID;
    _input->addJoystickMapping(Input::Joystick::JOYSTICK_1, Input::JoystickElement(Input::JoystickElementType::BUTTON_PRESS, 0), actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _freeFly = !_freeFly;
        if (!_freeFly)
            renderState().getCameraMgr().pushActiveCamera(_paddleCam);
        else
            renderState().getCameraMgr().popActiveCamera();
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);

    return actionID++;
}

bool PingPongScene::loadResources(bool continueOnErrors) {
    static const U32 lightMask = to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                 to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                 to_const_uint(SGNComponent::ComponentType::RENDERING);
    static const U32 normalMask = lightMask | 
                                  to_const_uint(SGNComponent::ComponentType::NAVIGATION);

    // Create a ball
    ResourceDescriptor minge("Ping Pong Ball");
    _ball = CreateResource<Sphere3D>(minge);
    _ball->setResolution(16);
    _ball->setRadius(0.1f);
    _ball->getMaterialTpl()->setDiffuse(vec4<F32>(0.4f, 0.4f, 0.4f, 1.0f));
    _ball->getMaterialTpl()->setShininess(36.8f);
    _ball->getMaterialTpl()->setSpecular(
        vec4<F32>(0.774597f, 0.774597f, 0.774597f, 1.0f));
    _ballSGN = _sceneGraph->getRoot().addNode(_ball, normalMask, PhysicsGroup::GROUP_KINEMATIC, "PingPongBallSGN");
    _ballSGN.lock()->get<PhysicsComponent>()->translate(vec3<F32>(0, 2, 2));

    /*ResourceDescriptor tempLight("Light Omni");
    tempLight.setEnumValue(LIGHT_TYPE_POINT);
    tempLight.setUserPtr(_lightPool.get());
    Light* light = CreateResource<Light>(tempLight);
    _sceneGraph->getRoot()->addNode(light, lightMask);
    light->setRange(30.0f);
    light->setCastShadows(false);
    light->setPosition(vec3<F32>(0, 6, 2));
    */
    const vec2<U16>& resolution = _GUI->getDisplayResolution();
    // Buttons and text labels
    _GUI->addButton(_ID("Serve"), "Serve",
                    vec2<I32>(to_int(resolution.width - 120),
                              to_int(resolution.height / 1.1f)),
                    vec2<U32>(100, 25),
                    DELEGATE_BIND(&PingPongScene::serveBall, this, std::placeholders::_1));

    _GUI->addText(_ID("Score"),
                  vec2<I32>(to_int(resolution.width - 120),
                            to_int(resolution.height / 1.3f)),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(255, 0, 0, 255),
                  Util::StringFormat("Score: %d", 0));

    _GUI->addText(_ID("Message"),
                  vec2<I32>(to_int(resolution.width - 120),
                            to_int(resolution.height / 1.5f)),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(255, 0, 0, 255),
                  "");
    _GUI->addText(_ID("insults"),
                  vec2<I32>(resolution.width / 4,
                            resolution.height / 3),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(0, 255, 0, 255),
                  "");
    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec4<U8>(0, 50, 255, 255),// Color
                  Util::StringFormat("FPS: %d", 0));  // Text and arguments
    // Add some taunts
    _quotes.push_back("Ha ha ... even Odin's laughin'!");
    _quotes.push_back("If you're a ping-pong player, I'm Jimmy Page");
    _quotes.push_back(
        "Ooolee, ole ole ole, see the ball? ... It's past your end");
    _quotes.push_back(
        "You're lucky the room's empty. I'd be so ashamed otherwise if I were "
        "you");
    _quotes.push_back("It's not the hard. Even a monkey can do it.");

    _guiTimers.push_back(0.0);  // Fps
    _taskTimers.push_back(0.0);  // Light

    return true;
}

};
#include "stdafx.h"

#include "Headers/PingPongScene.h"

#include "Core/Headers/StringHelper.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Managers/Headers/SceneManager.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Predefined/Headers/Box3D.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

namespace Divide {

namespace {
    I64 g_gameTaskID = 0;
};

PingPongScene::PingPongScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
    : Scene(context, cache, parent, name)
{
    _sideDrift = 0;
    _directionTowardsAdversary = true;
    _upwardsDirection = false;
    _touchedOwnTableHalf = false;
    _touchedAdversaryTableHalf = false;
    _lost = false;
    _ball = nullptr;
    _paddleCam = nullptr;
    _score = 0;
    _freeFly = false;
    _wasInFreeFly = false;
}

void PingPongScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimersMS[0] >= FpsDisplay) {
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _guiTimersMS[0] = 0.0;
    }
    Scene::processGUI(deltaTimeUS);
}

void PingPongScene::processTasks(const U64 deltaTimeUS) {
    static vec2<F32> _sunAngle =
        vec2<F32>(0.0f, Angle::to_RADIANS(45.0f));
    static bool direction = false;
    if (!direction) {
        _sunAngle.y += 0.005f;
        _sunAngle.x += 0.005f;
    } else {
        _sunAngle.y -= 0.005f;
        _sunAngle.x -= 0.005f;
    }

    if (_sunAngle.y <= Angle::to_RADIANS(25) ||
        _sunAngle.y >= Angle::to_RADIANS(70))
        direction = !direction;

    _sunvector =
        vec3<F32>(-cosf(_sunAngle.x) * sinf(_sunAngle.y), -cosf(_sunAngle.y),
                  -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
    constants.set("enable_sun", PushConstantType::BOOL, true);
    constants.set("sun_vector", PushConstantType::VEC3, _sunvector);
    constants.set("sun_colour", PushConstantType::VEC3, _sun->getNode<Light>()->getDiffuseColour());

    Scene::processTasks(deltaTimeUS);
}

void PingPongScene::resetGame() {
    _directionTowardsAdversary = true;
    _upwardsDirection = false;
    _touchedAdversaryTableHalf = false;
    _touchedOwnTableHalf = false;
    _lost = false;
    _sideDrift = 0;
    clearTasks();
    _ballSGN->get<TransformComponent>()->setPosition(vec3<F32>(0, 2, 2));
}

void PingPongScene::serveBall(I64 btnGUID) {
    _GUI->modifyText(_ID("insults"), "");
    resetGame();

    removeTask(g_gameTaskID);

    g_gameTaskID = registerTask(CreateTask(platformContext(), getGUID(),DELEGATE_BIND(&PingPongScene::test, this, std::placeholders::_1, Random(4), CallbackParam::TYPE_INTEGER)));
}

void PingPongScene::test(const Task& parentTask, AnyParam a, CallbackParam b) {
    while (!parentTask.stopRequested()) {
        bool updated = false;
        stringImpl message;
        TransformComponent* ballTransform =
            _ballSGN->get<TransformComponent>();
        vec3<F32> ballPosition = ballTransform->getPosition();

        SceneGraphNode* table(_sceneGraph->findNode("table"));
        SceneGraphNode* net(_sceneGraph->findNode("net"));
        SceneGraphNode* opponent(_sceneGraph->findNode("opponent"));
        SceneGraphNode* paddle(_sceneGraph->findNode("paddle"));

        vec3<F32> paddlePosition =
            paddle->get<TransformComponent>()->getPosition();
        vec3<F32> opponentPosition =
            opponent->get<TransformComponent>()->getPosition();
        vec3<F32> tablePosition =
            table->get<TransformComponent>()->getPosition();

        // Is the ball coming towards us or towards the opponent?
        _directionTowardsAdversary ? ballPosition.z -= 0.11f : ballPosition.z +=
                                                               0.11f;
        // Up or down?
        _upwardsDirection ? ballPosition.y += 0.084f : ballPosition.y -= 0.084f;

        // Is the ball moving to the right or to the left?
        ballPosition.x += _sideDrift * 0.15f;
        if (opponentPosition.x != ballPosition.x)
            opponent->get<TransformComponent>()->translateX(
                ballPosition.x - opponentPosition.x);

        ballTransform->translate(ballPosition - ballTransform->getPosition());

        // Did we hit the table? Bounce then ...
        if (table->get<BoundsComponent>()
                 ->getBoundingBox().collision(_ballSGN->get<BoundsComponent>()->getBoundingBox()))
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
        if (_ballSGN->get<BoundsComponent>()->getBoundingBox().collision(paddle->get<BoundsComponent>()->getBoundingBox())) {
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

        if (_ballSGN->get<BoundsComponent>()->getBoundingBox().collision(net->get<BoundsComponent>()->getBoundingBox())) {
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
            if (_ballSGN->get<BoundsComponent>()
                               ->getBoundingBox().collision(opponent->get<BoundsComponent>()
                                                                    ->getBoundingBox())) {
                _sideDrift =
                    ballPosition.x -
                    opponent->get<TransformComponent>()->getPosition().x;
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

void PingPongScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    if (_freeFly) {
        _wasInFreeFly = true;
        Scene::processInput(idx, deltaTimeUS);
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
    if (state().playerState(idx).angleLR() != MoveDirection::NONE) {
        _paddleCam->rotateYaw(Angle::DEGREES<F32>(state().playerState(idx).angleLR()));
    }
    if (state().playerState(idx).angleUD() != MoveDirection::NONE) {
        _paddleCam->rotatePitch(Angle::DEGREES<F32>(state().playerState(idx).angleUD()));
    }

    SceneGraphNode* paddle(_sceneGraph->findNode("paddle"));

    vec3<F32> pos = paddle->get<TransformComponent>()->getPosition();

    // Paddle movement is limited to the [-3,3] range except for Y-descent
    if (state().playerState(idx).moveFB() != MoveDirection::NONE) {
        if ((state().playerState(idx).moveFB() == MoveDirection::POSITIVE && pos.y >= 3) ||
            (state().playerState(idx).moveFB() == MoveDirection::NEGATIVE && pos.y <= 0.5f)) {
            Scene::processInput(idx, deltaTimeUS);
            return;
        }
        paddle->get<TransformComponent>()->translateY(to_I32(state().playerState(idx).moveFB()) / paddleMovementDivisor);
    }

    if (state().playerState(idx).moveLR() != MoveDirection::NONE) {
        // Left/right movement is flipped for proper control
        if ((state().playerState(idx).moveLR() == MoveDirection::NEGATIVE && pos.x >= 3) ||
            (state().playerState(idx).moveLR() == MoveDirection::POSITIVE && pos.x <= -3)) {
            Scene::processInput(idx, deltaTimeUS);
            return;
        }
        paddle->get<TransformComponent>()->translateX(to_I32(state().playerState(idx).moveLR()) / paddleMovementDivisor);
    }

    Scene::processInput(idx, deltaTimeUS);
}

bool PingPongScene::load(const stringImpl& name) {
    _freeFly = false;
    _wasInFreeFly = false;

    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    // Add a light
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _currentSky = addSky();

    return loadState;
}

U16 PingPongScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {serveBall(-1);});
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addJoystickMapping(Input::Joystick::JOYSTICK_1, Input::JoystickElement(Input::JoystickElementType::BUTTON_PRESS, 0), actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        _freeFly = !_freeFly;
        if (!_freeFly) {
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).overrideCamera(_paddleCam);
        } else {
            state().playerState(getPlayerIndexForDevice(param._deviceIndex)).overrideCamera(nullptr);
        }
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);

    return actionID++;
}

bool PingPongScene::loadResources(bool continueOnErrors) {
    static const U32 lightMask = to_base(ComponentType::TRANSFORM) |
                                 to_base(ComponentType::BOUNDS) |
                                 to_base(ComponentType::RENDERING);

    static const U32 normalMask = lightMask |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::NETWORKING);
        

    // Create a ball
    ResourceDescriptor minge("Ping Pong Ball");
    _ball = CreateResource<Sphere3D>(_resCache, minge);
    _ball->setResolution(16);
    _ball->setRadius(0.1f);
    _ball->getMaterialTpl()->setDiffuse(vec4<F32>(0.4f, 0.4f, 0.4f, 1.0f));
    _ball->getMaterialTpl()->setShininess(36.8f);
    _ball->getMaterialTpl()->setSpecular(
        vec4<F32>(0.774597f, 0.774597f, 0.774597f, 1.0f));
    _ballSGN = _sceneGraph->getRoot().addNode(_ball, normalMask, PhysicsGroup::GROUP_KINEMATIC, "PingPongBallSGN");
    _ballSGN->get<TransformComponent>()->translate(vec3<F32>(0, 2, 2));

    /*ResourceDescriptor tempLight("Light Omni");
    tempLight.setEnumValue(LIGHT_TYPE_POINT);
    tempLight.setUserPtr(_lightPool.get());
    Light* light = CreateResource<Light>(_resCache, tempLight);
    _sceneGraph->getRoot()->addNode(light, lightMask);
    light->setRange(30.0f);
    light->setCastShadows(false);
    light->setPosition(vec3<F32>(0, 6, 2));
    */
 
    // Add some taunts
    _quotes.push_back("Ha ha ... even Odin's laughin'!");
    _quotes.push_back("If you're a ping-pong player, I'm Jimmy Page");
    _quotes.push_back(
        "Ooolee, ole ole ole, see the ball? ... It's past your end");
    _quotes.push_back(
        "You're lucky the room's empty. I'd be so ashamed otherwise if I were "
        "you");
    _quotes.push_back("It's not the hard. Even a monkey can do it.");

    _guiTimersMS.push_back(0.0);  // Fps
    _taskTimers.push_back(0.0);  // Light

    _paddleCam = Camera::createCamera("paddleCam", Camera::CameraType::FREE_FLY);
    _paddleCam->fromCamera(*playerCamera());
    // Position the camera
    // renderState().getCamera().setPitch(-90);
    _paddleCam->lockMovement(true);
    _paddleCam->setEye(vec3<F32>(0, 2.5f, 6.5f));

    return true;
}

void PingPongScene::postLoadMainThread() {
    const vec2<U16>& resolution = _context.gfx().renderingResolution();
    // Buttons and text labels
    _GUI->addButton(_ID("Serve"), "Serve",
                    pixelPosition(to_I32(resolution.width - 120),
            to_I32(resolution.height / 1.1f)),
        pixelScale(100, 25),
        DELEGATE_BIND(&PingPongScene::serveBall, this, std::placeholders::_1));

    _GUI->addText(_ID("Score"),
                  pixelPosition(to_I32(resolution.width - 120),
            to_I32(resolution.height / 1.3f)),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(255, 0, 0, 255),
        Util::StringFormat("Score: %d", 0));

    _GUI->addText(_ID("Message"),
                  pixelPosition(to_I32(resolution.width - 120),
            to_I32(resolution.height / 1.5f)),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(255, 0, 0, 255),
        "");
    _GUI->addText(_ID("insults"),
                  pixelPosition(resolution.width / 4,
            resolution.height / 3),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(0, 255, 0, 255),
        "");
    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
                  pixelPosition(60, 60),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec4<U8>(0, 50, 255, 255),// Colour
        Util::StringFormat("FPS: %d", 0));  // Text and arguments

    Scene::postLoadMainThread();
}

};
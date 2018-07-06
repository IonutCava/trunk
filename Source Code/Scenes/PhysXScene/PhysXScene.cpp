#include "Headers/PhysXScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

REGISTER_SCENE(PhysXScene);

enum PhysXStateEnum {
    STATE_ADDING_ACTORS = 0,
    STATE_IDLE = 2,
    STATE_LOADING = 3
};

static std::atomic<PhysXStateEnum> s_sceneState;

// begin copy-paste
void PhysXScene::preRender() {
    _currentSky->getNode<Sky>()->setSunProperties(_sunvector,
                                                  _sun->getDiffuseColor());
}
//<<end copy-paste

void PhysXScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f",
                         Time::ApplicationTimer::getInstance().getFps(),
                         Time::ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d",
                         GFX_RENDER_BIN_SIZE);
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void PhysXScene::processInput(const U64 deltaTime) {}

bool PhysXScene::load(const stringImpl& name, GUI* const gui) {
    s_sceneState = STATE_LOADING;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec3<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y), -cosf(sunAngle.y),
                  -sinf(sunAngle.x) * sinf(sunAngle.y));
    _sun = addLight(LIGHT_TYPE_DIRECTIONAL,
               GET_ACTIVE_SCENEGRAPH().getRoot()).getNode<DirectionalLight>();
    _sun->setDirection(_sunvector);
    _currentSky =
        &addSky(CreateResource<Sky>(ResourceDescriptor("Default Sky")));
    s_sceneState = STATE_IDLE;
    return loadState;
}

bool PhysXScene::loadResources(bool continueOnErrors) {
    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 20),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  "FPS: %s", 0);  // Text and arguments
    _GUI->addText("RenderBinCount", vec2<I32>(60, 30), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  "Number of items in Render Bin: %d", 0);

    _guiTimers.push_back(0.0);  // Fps
    renderState().getCamera().setFixedYawAxis(false);
    renderState().getCamera().setRotation(-45 /*yaw*/, 10 /*pitch*/);
    renderState().getCamera().setEye(vec3<F32>(0, 30, -40));
    renderState().getCamera().setFixedYawAxis(true);
    ParamHandler::getInstance().setParam("rendering.enableFog", false);
    ParamHandler::getInstance().setParam("postProcessing.bloomFactor", 0.1f);
    return true;
}

bool PhysXScene::unload() { return Scene::unload(); }

void PhysXScene::createStack(U32 size) {
    F32 stackSize = size;
    F32 CubeSize = 1.0f;
    F32 Spacing = 0.0001f;
    vec3<F32> Pos(0, 10 + CubeSize, 0);
    F32 Offset = -stackSize * (CubeSize * 2.0f + Spacing) * 0.5f + 0;

    while (s_sceneState == STATE_ADDING_ACTORS)
        ;

    s_sceneState = STATE_ADDING_ACTORS;

    while (stackSize) {
        for (U16 i = 0; i < stackSize; i++) {
            Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
            PHYSICS_DEVICE.createBox(Pos, CubeSize);
        }
        Offset += CubeSize;
        Pos.y += (CubeSize * 2.0f + Spacing);
        stackSize--;
    }

    s_sceneState = STATE_IDLE;
}

void PhysXScene::createTower(U32 size) {
    while (s_sceneState == STATE_ADDING_ACTORS)
        ;
    s_sceneState = STATE_ADDING_ACTORS;

    for (U8 i = 0; i < size; i++)
        PHYSICS_DEVICE.createBox(vec3<F32>(0, 5.0f + 5 * i, 0), 0.5f);

    s_sceneState = STATE_IDLE;
}

bool PhysXScene::onKeyUp(const Input::KeyEvent& key) {
    switch (key._key) {
        default:
            break;
        case Input::KeyCode::KC_5: {
            _paramHandler.setParam(
                "simSpeed",
                IS_ZERO(_paramHandler.getParam<F32>("simSpeed")) ? 1.0f : 0.0f);
            PHYSICS_DEVICE.updateTimeStep();
        } break;
        case Input::KeyCode::KC_1: {
            static bool hasGroundPlane = false;
            if (!hasGroundPlane) {
                PHYSICS_DEVICE.createPlane(vec3<F32>(0, 0, 0),
                                           random(100.0f, 200.0f));
                hasGroundPlane = true;
            }
        } break;
        case Input::KeyCode::KC_2:
            PHYSICS_DEVICE.createBox(vec3<F32>(0, random(10, 30), 0),
                                     random(0.5f, 2.0f));
            break;
        case Input::KeyCode::KC_3: {
            Kernel& kernel = Application::getInstance().getKernel();
            Task_ptr e(
                kernel.AddTask(0, 1, DELEGATE_BIND(&PhysXScene::createTower,
                                                   this, (U32)random(5, 20))));
            registerTask(e);
            e->startTask();
        } break;
        case Input::KeyCode::KC_4: {
            Kernel& kernel = Application::getInstance().getKernel();
            Task_ptr e(
                kernel.AddTask(0, 1, DELEGATE_BIND(&PhysXScene::createStack,
                                                   this, (U32)random(5, 10))));
            registerTask(e);
            e->startTask();
        } break;
    }
    return Scene::onKeyUp(key);
}

bool PhysXScene::mouseMoved(const Input::MouseEvent& key) {
    return Scene::mouseMoved(key);
}

bool PhysXScene::mouseButtonReleased(const Input::MouseEvent& key,
                                     Input::MouseButton button) {
    return Scene::mouseButtonReleased(key, button);
    ;
}
};
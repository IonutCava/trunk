#include "Headers/PhysXScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

REGISTER_SCENE(PhysXScene);

enum class PhysXState : U32 {
    STATE_ADDING_ACTORS = 0,
    STATE_IDLE = 2,
    STATE_LOADING = 3
};

static std::atomic<PhysXState> s_sceneState;

// begin copy-paste
void PhysXScene::preRender() {
    _currentSky.lock()->getNode<Sky>()->setSunProperties(_sunvector, _sun.lock()->getNode<Light>()->getDiffuseColor());
}
//<<end copy-paste

void PhysXScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        _GUI->modifyText("fpsDisplay",
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::getInstance().getFps(),
                                            Time::ApplicationTimer::getInstance().getFrameTime()));
        _GUI->modifyText("RenderBinCount",
                         Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                                            GFX_RENDER_BIN_SIZE, GFX_HIZ_CULL_COUNT));
        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void PhysXScene::processInput(const U64 deltaTime) {}

bool PhysXScene::load(const stringImpl& name, GUI* const gui) {
    s_sceneState = PhysXState::STATE_LOADING;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::DegreesToRadians(45.0f));
    _sunvector =
        vec3<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y), -cosf(sunAngle.y),
                  -sinf(sunAngle.x) * sinf(sunAngle.y));
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph.getRoot());
    _sun.lock()->get<PhysicsComponent>()->setPosition(_sunvector);
    _currentSky = addSky();

    SceneInput::PressReleaseActions cbks;
    cbks.second = [this]() {
        if (!_hasGroundPlane) {
            PHYSICS_DEVICE.createPlane(vec3<F32>(0, 0, 0),
                                       Random(100, 200));
            _hasGroundPlane = true;
        }
    };
    _input->addKeyMapping(Input::KeyCode::KC_1, cbks);
    cbks.second =
        DELEGATE_BIND(&PXDevice::createBox, &PHYSICS_DEVICE,
                      vec3<F32>(0, Random(10, 30), 0), Random(0.5f, 2.0f));
    _input->addKeyMapping(Input::KeyCode::KC_2, cbks);
    cbks.second = [this]() {
        Kernel& kernel = Application::getInstance().getKernel();
        TaskHandle e(kernel.AddTask(getGUID(), DELEGATE_BIND(&PhysXScene::createTower, this, std::placeholders::_1, to_uint(Random(5, 20)))));
        e.startTask();
        registerTask(e);
    };
    _input->addKeyMapping(Input::KeyCode::KC_3, cbks);
    cbks.second = [this]() {
        Kernel& kernel = Application::getInstance().getKernel();
        TaskHandle e(kernel.AddTask(getGUID(), DELEGATE_BIND(&PhysXScene::createStack, this, std::placeholders::_1, to_uint(Random(5, 10)))));
        e.startTask();
        registerTask(e);
    };
    _input->addKeyMapping(Input::KeyCode::KC_4, cbks);
    cbks.second = []() {
        ParamHandler::getInstance().setParam("simSpeed",
            IS_ZERO(ParamHandler::getInstance().getParam<F32>("simSpeed"))
                ? 1.0f
                : 0.0f);
        PHYSICS_DEVICE.updateTimeStep();
    };
    _input->addKeyMapping(Input::KeyCode::KC_5, cbks);

    s_sceneState = PhysXState::STATE_IDLE;
    return loadState;
}

bool PhysXScene::loadResources(bool continueOnErrors) {
    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 20),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  Util::StringFormat("FPS: %d", 0));  // Text and arguments
    _GUI->addText("RenderBinCount", vec2<I32>(60, 30), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  Util::StringFormat("Number of items in Render Bin: %d", 0));

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

void PhysXScene::createStack(bool stopRequested, U32 size) {
    U32 stackSize = size;
    F32 CubeSize = 1.0f;
    F32 Spacing = 0.0001f;
    vec3<F32> Pos(0, 10 + CubeSize, 0);
    F32 Offset = -1 * stackSize * (CubeSize * 2.0f + Spacing) * 0.5f;

    WAIT_FOR_CONDITION(s_sceneState != PhysXState::STATE_ADDING_ACTORS);

    s_sceneState = PhysXState::STATE_ADDING_ACTORS;

    while (stackSize) {
        if (stopRequested){
            return;
        }
        for (U16 i = 0; i < stackSize; i++) {
            Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
            PHYSICS_DEVICE.createBox(Pos, CubeSize);
        }
        Offset += CubeSize;
        Pos.y += (CubeSize * 2.0f + Spacing);
        stackSize--;
    }

    s_sceneState = PhysXState::STATE_IDLE;
}

void PhysXScene::createTower(bool stopRequested, U32 size) {
    while (s_sceneState == PhysXState::STATE_ADDING_ACTORS);
        
    s_sceneState = PhysXState::STATE_ADDING_ACTORS;

    for (U8 i = 0; i < size; i++) {
        if (stopRequested) {
            return;
        }
        PHYSICS_DEVICE.createBox(vec3<F32>(0, 5.0f + 5 * i, 0), 0.5f);
    }
    s_sceneState = PhysXState::STATE_IDLE;
}

};
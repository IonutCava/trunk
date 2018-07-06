#include "stdafx.h"

#include "Headers/PhysXScene.h"

#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Headers/Configuration.h"
#include "Core/Headers/StringHelper.h"
#include "Core/Headers/ParamHandler.h"

namespace Divide {

enum class PhysXState : U32 {
    STATE_ADDING_ACTORS = 0,
    STATE_IDLE = 2,
    STATE_LOADING = 3
};

namespace {
    std::atomic<PhysXState> s_sceneState;
};

void PhysXScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
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

void PhysXScene::processInput(PlayerIndex idx, const U64 deltaTimeUS) {
    PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
    constants.set("enable_sun", GFX::PushConstantType::BOOL, true);
    constants.set("sun_vector", GFX::PushConstantType::VEC3, _sunvector);
    constants.set("sun_colour", GFX::PushConstantType::VEC3, _sun->getNode<Light>()->getDiffuseColour());

    Scene::processInput(idx, deltaTimeUS);
}

bool PhysXScene::load(const stringImpl& name) {
    s_sceneState = PhysXState::STATE_LOADING;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    // Add a light
    vec2<F32> sunAngle(0.0f, Angle::to_RADIANS(45.0f));
    _sunvector =
        vec3<F32>(-cosf(sunAngle.x) * sinf(sunAngle.y), -cosf(sunAngle.y),
                  -sinf(sunAngle.x) * sinf(sunAngle.y));
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    _sun->get<TransformComponent>()->setPosition(_sunvector);
    _currentSky = addSky();

    s_sceneState = PhysXState::STATE_IDLE;
    return loadState;
}

U16 PhysXScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        if (!_hasGroundPlane) {
            DIVIDE_UNEXPECTED_CALL();
            // register ground plane
            _hasGroundPlane = true;
        }
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_1, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param){
        // Create Box
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_2, actions);
    actionID++;


    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        TaskHandle e(CreateTask(context(), getGUID(), DELEGATE_BIND(&PhysXScene::createTower, this, std::placeholders::_1, to_U32(Random(5, 20)))));
        registerTask(e);
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_3, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        TaskHandle e(CreateTask(context(), getGUID(), DELEGATE_BIND(&PhysXScene::createStack, this, std::placeholders::_1, to_U32(Random(5, 10)))));
        registerTask(e);
    });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_4, actions);

    return actionID++;
}

bool PhysXScene::loadResources(bool continueOnErrors) {
    _guiTimersMS.push_back(0.0);  // Fps
    playerCamera()->setFixedYawAxis(false);
    playerCamera()->setRotation(-45 /*yaw*/, 10 /*pitch*/);
    playerCamera()->setEye(vec3<F32>(0, 30, -40));
    playerCamera()->setFixedYawAxis(true);
    _context.config().rendering.enableFog = false;
    _context.config().rendering.bloomFactor = 0.1f;
    return true;
}

void PhysXScene::postLoadMainThread() {
    _GUI->addText("fpsDisplay",  // Unique ID
        pixelPosition(60, 20),  // Position
        Font::DIVIDE_DEFAULT,  // Font
                  UColour(0, 64, 255, 255),  // Colour
        Util::StringFormat("FPS: %d", 0));  // Text and arguments
    _GUI->addText("RenderBinCount", pixelPosition(60, 30), Font::DIVIDE_DEFAULT,
                  UColour(164, 64, 64, 255),
        Util::StringFormat("Number of items in Render Bin: %d", 0));

    Scene::postLoadMainThread();
}

bool PhysXScene::unload() { return Scene::unload(); }

void PhysXScene::createStack(const Task& parentTask, U32 size) {
    U32 stackSize = size;
    F32 CubeSize = 1.0f;
    F32 Spacing = 0.0001f;
    vec3<F32> Pos(0, 10 + CubeSize, 0);
    F32 Offset = -1 * stackSize * (CubeSize * 2.0f + Spacing) * 0.5f;

    WAIT_FOR_CONDITION(s_sceneState != PhysXState::STATE_ADDING_ACTORS);

    s_sceneState = PhysXState::STATE_ADDING_ACTORS;

    while (stackSize) {
        if (parentTask.stopRequested()){
            return;
        }
        for (U16 i = 0; i < stackSize; i++) {
            Pos.x = Offset + i * (CubeSize * 2.0f + Spacing);
            DIVIDE_UNEXPECTED_CALL();
            //CREATE BOX
        }
        Offset += CubeSize;
        Pos.y += (CubeSize * 2.0f + Spacing);
        stackSize--;
    }

    s_sceneState = PhysXState::STATE_IDLE;
}

void PhysXScene::createTower(const Task& parentTask, U32 size) {
    while (s_sceneState == PhysXState::STATE_ADDING_ACTORS);
        
    s_sceneState = PhysXState::STATE_ADDING_ACTORS;

    for (U8 i = 0; i < size; i++) {
        if (parentTask.stopRequested()) {
            return;
        }
        DIVIDE_UNEXPECTED_CALL();
        //CREATE BOX *vec3<F32>(0, 5.0f + 5 * i, 0), 0.5f
    }
    s_sceneState = PhysXState::STATE_IDLE;
}

};
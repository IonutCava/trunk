#include "Headers/WarScene.h"
#include "Headers/WarSceneAIProcessor.h"

#include "GUI/Headers/GUIMessageBox.h"
#include "Geometry/Material/Headers/Material.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/AIManager.h"

#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicTimeUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicColorUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleEulerUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleFloorUpdater.h"

#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleBoxGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleColorGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleVelocityGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleTimeGenerator.h"

namespace Divide {

REGISTER_SCENE(WarScene);

namespace {
    static vec2<F32> g_sunAngle(0.0f, Angle::DegreesToRadians(45.0f));
    static bool g_direction = false;
    U64 elapsedGameTimeUs = 0;
};

WarScene::WarScene()
    : Scene(),
    _sun(nullptr),
    _infoBox(nullptr),
    _sceneReady(false),
    _lastNavMeshBuildTime(0UL)
{
    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = nullptr;
    }

    _resetUnits = false;

    addSelectionCallback([&]() {
        SceneGraphNode_ptr selection(_currentSelection.lock());
        if (selection) {
            _GUI->modifyText("entityState", selection->getName().c_str());
        } else {
            _GUI->modifyText("entityState", "");
        }
    });

    _targetLines = GFX_DEVICE.getOrCreatePrimitive(false);

    _runCount = 0;
    _timeLimitMinutes = 5;
    _scoreLimit = 3;
    _elapsedGameTime = 0;
}

WarScene::~WarScene()
{
    _targetLines->_canZombify = true;
}

void WarScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        const Camera& cam = renderState().getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f",
                         Time::ApplicationTimer::getInstance().getFps(),
                         Time::ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d",
                         GFX_RENDER_BIN_SIZE);
        _GUI->modifyText("camPosition",
                         "Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: "
                         "%5.2f | Yaw: %5.2f]",
                         eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);

        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= Time::SecondsToMilliseconds(1)) {
        SceneGraphNode_ptr selection(_currentSelection.lock());
        if (selection) {
            AI::AIEntity* entity = findAI(selection);
            if (entity) {
                _GUI->modifyText("entityState", entity->toString().c_str());
            }
        }
    }

    if (_guiTimers[2] >= 66) {
        U32 elapsedTimeMinutes = (Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) / 60) % 60;
        U32 elapsedTimeSeconds = Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) % 60;
        U32 elapsedTimeMilliseconds = Time::MicrosecondsToMilliseconds<U32>(_elapsedGameTime) % 1000;


        U32 limitTimeMinutes = _timeLimitMinutes;
        U32 limitTimeSeconds = 0;
        U32 limitTimeMilliseconds = 0;

        _GUI->modifyText("scoreDisplay", "Score: A -  %d B - %d [Limit: %d]\nElapsed game time [ %d:%d:%d / %d:%d:%d]",
            AI::WarSceneAIProcessor::getScore(0),
            AI::WarSceneAIProcessor::getScore(1),
            _scoreLimit,
            elapsedTimeMinutes,
            elapsedTimeSeconds,
            elapsedTimeMilliseconds,
            limitTimeMinutes,
            limitTimeSeconds,
            limitTimeMilliseconds);

        _guiTimers[2] = 0.0;
    }

    Scene::processGUI(deltaTime);
}

void WarScene::processTasks(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

    D32 SunTimer = Time::Milliseconds(10);
    D32 AnimationTimer1 = Time::SecondsToMilliseconds(5);
    D32 AnimationTimer2 = Time::SecondsToMilliseconds(10);

    if (_taskTimers[0] >= SunTimer) {
        if (!g_direction) {
            g_sunAngle.y += 0.00025f;
            g_sunAngle.x += 0.00025f;
        } else {
            g_sunAngle.y -= 0.00025f;
            g_sunAngle.x -= 0.00025f;
        }

        if (!IS_IN_RANGE_INCLUSIVE(g_sunAngle.y, 
                                   Angle::DegreesToRadians(25.0f),
                                   Angle::DegreesToRadians(55.0f))) {
            g_direction = !g_direction;
        }

        vec3<F32> sunVector(-cosf(g_sunAngle.x) * sinf(g_sunAngle.y),
                            -cosf(g_sunAngle.y),
                            -sinf(g_sunAngle.x) * sinf(g_sunAngle.y));

       _sun->setDirection(sunVector);
       vec4<F32> sunColor = vec4<F32>(1.0f, 1.0f, 0.2f, 1.0f);

       _sun->setDiffuseColor(sunColor);
        _currentSky.lock()->getNode<Sky>()->setSunProperties(sunVector,
            _sun->getDiffuseColor());

        _taskTimers[0] = 0.0;
    }

    if (_taskTimers[1] >= AnimationTimer1) {
        for (NPC* const npc : _armyNPCs[0]) {
            npc->playNextAnimation();
        }
        _taskTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= AnimationTimer2) {
        for (NPC* const npc : _armyNPCs[1]) {
            npc->playNextAnimation();
        }
        _taskTimers[2] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

void WarScene::updateSceneStateInternal(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

    if (_resetUnits) {
        resetUnits();
        _resetUnits = false;
    }

    if (!AI::AIManager::getInstance().getNavMesh(
            _army[0][0]->getAgentRadiusCategory())) {
        return;
    }

    // renderState().drawDebugLines(true);
    vec3<F32> tempDestination;
    vec4<U8> redLine(255,0,0,128);
    vec4<U8> blueLine(0,0,255,128);
    vectorImpl<Line> paths;
    paths.reserve(_army[0].size() + _army[1].size());
    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* const character : _army[i]) {
            if (!character->getUnitRef()->getBoundNode().lock()->isActive()) {
                continue;
            }
            tempDestination.set(character->getDestination());
            if (!tempDestination.isZeroLength()) {
                vectorAlg::emplace_back(paths,
                    character->getPosition(),
                    tempDestination,
                    i == 0 ? blueLine : redLine,
                    i == 0 ? blueLine / 2 : redLine / 2);
            }
        }
    }
    GFX_DEVICE.drawLines(*_targetLines, paths, vec4<I32>());

    if (!AI::AIManager::getInstance().updatePaused()) {
        _elapsedGameTime += deltaTime;
        checkGameCompletion();
    }
}

bool WarScene::load(const stringImpl& name, GUI* const gui) {
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    _sun = addLight(LightType::DIRECTIONAL,
        GET_ACTIVE_SCENEGRAPH().getRoot())->getNode<DirectionalLight>();
    // Add a skybox
    _currentSky = addSky();
    // Position camera
    renderState().getCamera().setEye(vec3<F32>(43.13f, 147.09f, -4.41f));
    renderState().getCamera().setGlobalRotation(-90 /*yaw*/, 59.21 /*pitch*/);
    _sun->csmSplitCount(3);  // 3 splits
    _sun->csmSplitLogFactor(0.925f);
    _sun->csmNearClipOffset(25.0f);
    // Add some obstacles
    SceneGraphNode_ptr cylinder[5];
    cylinder[0] = _sceneGraph->findNode("cylinderC").lock();
    cylinder[1] = _sceneGraph->findNode("cylinderNW").lock();
    cylinder[2] = _sceneGraph->findNode("cylinderNE").lock();
    cylinder[3] = _sceneGraph->findNode("cylinderSW").lock();
    cylinder[4] = _sceneGraph->findNode("cylinderSE").lock();

    for (U8 i = 0; i < 5; ++i) {
        RenderingComponent* const renderable =
            cylinder[i]->getChildren().front()->getComponent<RenderingComponent>();
        renderable->getMaterialInstance()->setDoubleSided(true);
        cylinder[i]->getChildren().front()->getNode()->getMaterialTpl()->setDoubleSided(true);
    }

    SceneNode* cylinderMeshNW = cylinder[1]->getNode();
    SceneNode* cylinderMeshNE = cylinder[2]->getNode();
    SceneNode* cylinderMeshSW = cylinder[3]->getNode();
    SceneNode* cylinderMeshSE = cylinder[4]->getNode();

    stringImpl currentName;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode_ptr baseNode;

    std::pair<I32, I32> currentPos;
    for (U8 i = 0; i < 40; ++i) {
        if (i < 10) {
            baseNode = cylinder[1];
            currentMesh = cylinderMeshNW;
            currentName = "Cylinder_NW_" + std::to_string((I32)i);
            currentPos.first = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        }
        else if (i >= 10 && i < 20) {
            baseNode = cylinder[2];
            currentMesh = cylinderMeshNE;
            currentName = "Cylinder_NE_" + std::to_string((I32)i);
            currentPos.first = 200 - 40 * (i % 10) - 50;
            currentPos.second = -200 + 40 * (i % 10) + 50;
        }
        else if (i >= 20 && i < 30) {
            baseNode = cylinder[3];
            currentMesh = cylinderMeshSW;
            currentName = "Cylinder_SW_" + std::to_string((I32)i);
            currentPos.first = -200 + 40 * (i % 20) + 50;
            currentPos.second = 200 - 40 * (i % 20) - 50;
        }
        else {
            baseNode = cylinder[4];
            currentMesh = cylinderMeshSE;
            currentName = "Cylinder_SE_" + std::to_string((I32)i);
            currentPos.first = 200 - 40 * (i % 30) - 50;
            currentPos.second = 200 - 40 * (i % 30) - 50;
        }

        SceneGraphNode_ptr crtNode = _sceneGraph->getRoot()->addNode(*currentMesh,
            currentName);
        crtNode->setSelectable(true);
        crtNode->usageContext(baseNode->usageContext());
        PhysicsComponent* pComp = crtNode->getComponent<PhysicsComponent>();
        NavigationComponent* nComp =
            crtNode->getComponent<NavigationComponent>();
        pComp->physicsGroup(
            baseNode->getComponent<PhysicsComponent>()->physicsGroup());
        nComp->navigationContext(
            baseNode->getComponent<NavigationComponent>()->navigationContext());
        nComp->navigationDetailOverride(
            baseNode->getComponent<NavigationComponent>()
            ->navMeshDetailOverride());

        pComp->setScale(baseNode->getComponent<PhysicsComponent>()->getScale());
        pComp->setPosition(
            vec3<F32>(to_float(currentPos.first), -0.01f, to_float(currentPos.second)));
    }

    SceneGraphNode_ptr flag;
    flag = _sceneGraph->findNode("flag").lock();
    RenderingComponent* const renderable = flag->getChildren().front()->getComponent<RenderingComponent>();
    renderable->getMaterialInstance()->setDoubleSided(true);
    Material* mat = flag->getChildren().front()->getNode()->getMaterialTpl();
    mat->setDoubleSided(true);
    mat->setAmbient(vec4<F32>(0.01f));
    flag->setActive(false);
    SceneNode* flagNode = flag->getNode();

    _flag[0] = _sceneGraph->getRoot()->addNode(*flagNode, "Team1Flag");

    SceneGraphNode_ptr flag0(_flag[0].lock());
    flag0->setSelectable(false);
    flag0->usageContext(flag->usageContext());
    PhysicsComponent* flagPComp = flag0->getComponent<PhysicsComponent>();
    NavigationComponent* flagNComp =
        flag0->getComponent<NavigationComponent>();
    RenderingComponent* flagRComp =
        flag0->getChildren().front()->getComponent<RenderingComponent>();

    flagPComp->physicsGroup(
        flag->getComponent<PhysicsComponent>()->physicsGroup());
    flagPComp->setScale(flag->getComponent<PhysicsComponent>()->getScale());
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, -206.0f));

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(vec4<F32>(0.0f, 0.0f, 1.0f, 1.0f));

    _flag[1] = _sceneGraph->getRoot()->addNode(*flagNode, "Team2Flag");
    SceneGraphNode_ptr flag1(_flag[1].lock());
    flag1->setSelectable(false);
    flag1->usageContext(flag->usageContext());

    flagPComp = flag1->getComponent<PhysicsComponent>();
    flagNComp = flag1->getComponent<NavigationComponent>();
    flagRComp = flag1->getChildren().front()->getComponent<RenderingComponent>();

    flagPComp->physicsGroup(
        flag->getComponent<PhysicsComponent>()->physicsGroup());

    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, 206.0f));
    flagPComp->setScale(flag->getComponent<PhysicsComponent>()->getScale());

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(vec4<F32>(1.0f, 0.0f, 0.0f, 1.0f));

    AI::WarSceneAIProcessor::registerFlags(_flag[0], _flag[1]);

    AI::WarSceneAIProcessor::registerScoreCallback([&](U8 teamID, const stringImpl& unitName) {
        registerPoint(teamID, unitName);
    });

    AI::WarSceneAIProcessor::registerMessageCallback([&](U8 eventID, const stringImpl& unitName) {
        printMessage(eventID, unitName);
    });
    
#ifdef _DEBUG
    const U32 particleCount = 750;
#else
    const U32 particleCount = 20000;
#endif
    const F32 emitRate = particleCount / 4;

    std::shared_ptr<ParticleData> particles = 
        std::make_shared<ParticleData>(particleCount,
                                       to_uint(ParticleData::Properties::PROPERTIES_POS) |
                                       to_uint(ParticleData::Properties::PROPERTIES_VEL) |
                                       to_uint(ParticleData::Properties::PROPERTIES_ACC) | 
                                       to_uint(ParticleData::Properties::PROPERTIES_COLOR) |
                                       to_uint(ParticleData::Properties::PROPERTIES_COLOR_TRANS));
    particles->_textureFileName = "particle.DDS";

    std::shared_ptr<ParticleSource> particleSource =  std::make_shared<ParticleSource>(emitRate);

    std::shared_ptr<ParticleBoxGenerator> boxGenerator = std::make_shared<ParticleBoxGenerator>();
    boxGenerator->_maxStartPosOffset.set(0.3f, 0.0f, 0.3f, 1.0f);
    particleSource->addGenerator(boxGenerator);

    std::shared_ptr<ParticleColorGenerator> colGenerator = std::make_shared<ParticleColorGenerator>();
    colGenerator->_minStartCol.set(Util::ToByteColor(vec4<F32>(0.7f, 0.4f, 0.4f, 1.0f)));
    colGenerator->_maxStartCol.set(Util::ToByteColor(vec4<F32>(1.0f, 0.8f, 0.8f, 1.0f)));
    colGenerator->_minEndCol.set(Util::ToByteColor(vec4<F32>(0.5f, 0.2f, 0.2f, 0.5f)));
    colGenerator->_maxEndCol.set(Util::ToByteColor(vec4<F32>(0.7f, 0.5f, 0.5f, 0.75f)));
    particleSource->addGenerator(colGenerator);

    std::shared_ptr<ParticleVelocityGenerator> velGenerator = std::make_shared<ParticleVelocityGenerator>();
    velGenerator->_minStartVel.set(-1.0f, 0.22f, -1.0f, 0.0f);
    velGenerator->_maxStartVel.set(1.0f, 3.45f, 1.0f, 0.0f);
    particleSource->addGenerator(velGenerator);

    std::shared_ptr<ParticleTimeGenerator> timeGenerator = std::make_shared<ParticleTimeGenerator>();
    timeGenerator->_minTime = 8.5f;
    timeGenerator->_maxTime = 20.5f;
    particleSource->addGenerator(timeGenerator);

    SceneGraphNode_ptr testSGN = addParticleEmitter("TESTPARTICLES", particles, _sceneGraph->getRoot());

    ParticleEmitter* test = testSGN->getNode<ParticleEmitter>();
    testSGN->getComponent<PhysicsComponent>()->translateY(10);
    test->setDrawImpostor(true);
    test->enableEmitter(true);
    test->addSource(particleSource);
    boxGenerator->_pos.set(testSGN->getComponent<PhysicsComponent>()->getPosition());

    std::shared_ptr<ParticleEulerUpdater> eulerUpdater = std::make_shared<ParticleEulerUpdater>();
    eulerUpdater->_globalAcceleration.set(0.0f, -20.0f, 0.0f);
    test->addUpdater(eulerUpdater);
    std::shared_ptr<ParticleFloorUpdater> floorUpdater = std::make_shared<ParticleFloorUpdater>();
    floorUpdater->_bounceFactor = 0.65f;
    test->addUpdater(floorUpdater);
    test->addUpdater(std::make_shared<ParticleBasicTimeUpdater>());
    test->addUpdater(std::make_shared<ParticleBasicColorUpdater>());
    
    state().generalVisibility(state().generalVisibility() * 2);

    Application::getInstance()
        .getKernel()
        .getCameraMgr()
        .getActiveCamera()
        ->setHorizontalFoV(110);

    SceneInput::PressReleaseActions cbks;
    cbks.second = DELEGATE_BIND(&WarScene::toggleCamera, this);
    _input->addKeyMapping(Input::KeyCode::KC_TAB, cbks);

    cbks.second = DELEGATE_BIND(&WarScene::registerPoint, this, 0, "");
    _input->addKeyMapping(Input::KeyCode::KC_1, cbks);

    cbks.second = DELEGATE_BIND(&WarScene::registerPoint, this, 1, "");
    _input->addKeyMapping(Input::KeyCode::KC_2, cbks);

    cbks.second = [](){DIVIDE_ASSERT(false, "Test Assert"); };
    _input->addKeyMapping(Input::KeyCode::KC_5, cbks);

    _sceneReady = true;
    return loadState;
}

void WarScene::toggleCamera() {
    static bool fpsCameraActive = false;
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    if (_currentSelection.lock()) {
        if (flyCameraActive) {
            if (fpsCameraActive) {
                renderState().getCameraMgr().popActiveCamera();
            }
            renderState().getCameraMgr().pushActiveCamera("tpsCamera");
            static_cast<ThirdPersonCamera&>(renderState().getCamera())
                .setTarget(_currentSelection);
            flyCameraActive = false;
            tpsCameraActive = true;
            return;
        }
    }
    if (tpsCameraActive) {
        renderState().getCameraMgr().pushActiveCamera("defaultCamera");
        tpsCameraActive = false;
        flyCameraActive = true;
    }
}

bool WarScene::loadResources(bool continueOnErrors) {
    const vec2<U16>& resolution
        = Application::getInstance().getWindowManager().getResolution();

    _GUI->addButton("Simulate", "Simulate",
                    vec2<I32>(resolution.width - 220, 60),
                    vec2<U32>(100, 25), vec3<F32>(0.65f),
                    DELEGATE_BIND(&WarScene::startSimulation, this));

    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 63),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                 "fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", 0.0f, 0.0f);  // Text and arguments
    _GUI->addText("RenderBinCount",
                  vec2<I32>(60, 83),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  "Number of items in Render Bin: %d", 0);
    _GUI->addText("camPosition", vec2<I32>(60, 103),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | "
                  "Yaw: %5.2f]",
                  renderState().getCamera().getEye().x,
                  renderState().getCamera().getEye().y,
                  renderState().getCamera().getEye().z,
                  renderState().getCamera().getEuler().pitch,
                  renderState().getCamera().getEuler().yaw);

    _GUI->addText("scoreDisplay",
        vec2<I32>(60, 123),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec3<F32>(0.2f, 0.8f, 0.2f),  // Color
        "Score: A -  %d B - %d", 0, 0);  // Text and arguments

    _GUI->addText("entityState", vec2<I32>(60, 163), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.0f, 0.0f, 0.0f),
                  "");

    _infoBox = _GUI->addMsgBox("infoBox", "Info", "Blabla");
    // Add a first person camera
    Camera* cam = MemoryManager_NEW FirstPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(10.0f);
    cam->setTurnSpeedFactor(10.0f);
    renderState().getCameraMgr().addNewCamera("fpsCamera", cam);
    // Add a third person camera
    cam = MemoryManager_NEW ThirdPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);
    renderState().getCameraMgr().addNewCamera("tpsCamera", cam);

    _guiTimers.push_back(0.0);  // Fps
    _guiTimers.push_back(0.0);  // AI info
    _guiTimers.push_back(0.0);  // Game info
    _taskTimers.push_back(0.0); // Sun animation
    _taskTimers.push_back(0.0); // animation team 1
    _taskTimers.push_back(0.0); // animation team 2
    return true;
}

};

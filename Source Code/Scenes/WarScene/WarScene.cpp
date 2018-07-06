#include "Headers/WarScene.h"
#include "Headers/WarSceneAISceneImpl.h"
#include "AESOPActions/Headers/WarSceneActions.h"

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

WarScene::WarScene()
    : Scene(),
      _sun(nullptr),
      _infoBox(nullptr),
      _bobNode(nullptr),
      _bobNodeBody(nullptr),
      _lampLightNode(nullptr),
      _lampTransform(nullptr),
      _lampTransformNode(nullptr),
      _groundPlaceholder(nullptr),
      _sceneReady(false),
      _lastNavMeshBuildTime(0UL) {
    for (U8 i = 0; i < 2; ++i) {
        _flag[i] = nullptr;
        _faction[i] = nullptr;
    }
}

WarScene::~WarScene() {}

void WarScene::processGUI(const U64 deltaTime) {
    D32 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        const Camera& cam = renderState().getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        // const vec3<F32>& lampPos =
        // _lampLightNode->getComponent<PhysicsComponent>()->getPosition();
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f",
                         Time::ApplicationTimer::getInstance().getFps(),
                         Time::ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d",
                         GFX_RENDER_BIN_SIZE);
        _GUI->modifyText("camPosition",
                         "Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: "
                         "%5.2f | Yaw: %5.2f]",
                         eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);
        /*_GUI->modifyText("lampPosition","Lamp Position  [ X: %5.2f | Y: %5.2f
        | Z: %5.2f ]",
        lampPos.x, lampPos.y, lampPos.z);*/

        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void WarScene::processTasks(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

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

    _sun->setDirection(_sunvector);
    _currentSky->getNode<Sky>()->setSunProperties(_sunvector,
                                                  _sun->getDiffuseColor());

    D32 BobTimer = Time::SecondsToMilliseconds(5);
    D32 DwarfTimer = Time::SecondsToMilliseconds(8);
    D32 BullTimer = Time::SecondsToMilliseconds(10);

    if (_taskTimers[0] >= BobTimer) {
        if (_bobNode)
            _bobNode->getComponent<AnimationComponent>()->playNextAnimation();

        _taskTimers[0] = 0.0;
    }
    if (_taskTimers[1] >= DwarfTimer) {
        SceneGraphNode* dwarf = _sceneGraph.findNode("Soldier1");
        if (dwarf)
            dwarf->getComponent<AnimationComponent>()->playNextAnimation();
        _taskTimers[1] = 0.0;
    }
    if (_taskTimers[2] >= BullTimer) {
        SceneGraphNode* bull = _sceneGraph.findNode("Soldier2");
        if (bull) bull->getComponent<AnimationComponent>()->playNextAnimation();
        _taskTimers[2] = 0.0;
    }
    Scene::processTasks(deltaTime);
}

static std::atomic_bool navMeshStarted;

void navMeshCreationCompleteCallback(AI::AIEntity::PresetAgentRadius radius,
                                     AI::Navigation::NavigationMesh* navMesh) {
    navMesh->save();
    AI::AIManager::getInstance().addNavMesh(radius, navMesh);
}

void WarScene::startSimulation() {
    AI::AIManager::getInstance().pauseUpdate(true);
    _infoBox->setTitle("NavMesh state");
    _infoBox->setMessageType(GUIMessageBox::MESSAGE_INFO);
    bool previousMesh = false;
    bool loadedFromFile = true;
    U64 currentTime = Time::ElapsedMicroseconds(true);
    U64 diffTime = currentTime - _lastNavMeshBuildTime;
    if (diffTime > Time::SecondsToMicroseconds(10)) {
        AI::Navigation::NavigationMesh* navMesh =
            AI::AIManager::getInstance().getNavMesh(
                _army[0][0]->getAgentRadiusCategory());
        if (navMesh) {
            previousMesh = true;
            AI::AIManager::getInstance().destroyNavMesh(
                _army[0][0]->getAgentRadiusCategory());
        }
        navMesh = MemoryManager_NEW AI::Navigation::NavigationMesh();
        navMesh->setFileName(GET_ACTIVE_SCENE()->getName());

        if (!navMesh->load(nullptr)) {
            loadedFromFile = false;
            navMesh->build(
                nullptr,
                DELEGATE_BIND(navMeshCreationCompleteCallback,
                              _army[0][0]->getAgentRadiusCategory(), navMesh));
        } else {
            AI::AIManager::getInstance().addNavMesh(
                _army[0][0]->getAgentRadiusCategory(), navMesh);
#ifdef _DEBUG
            navMesh->debugDraw(true);
            renderState().drawDebugTargetLines(true);
#endif
        }

        if (previousMesh) {
            if (loadedFromFile) {
                _infoBox->setMessage(
                    "Re-loaded the navigation mesh from file!");
            } else {
                _infoBox->setMessage(
                    "Re-building the navigation mesh in a background thread!");
            }
        } else {
            if (loadedFromFile) {
                _infoBox->setMessage("Navigation mesh loaded from file!");
            } else {
                _infoBox->setMessage(
                    "Navigation mesh building in a background thread!");
            }
        }
        _infoBox->show();
        navMeshStarted = true;
        _lastNavMeshBuildTime = currentTime;
        for (U8 i = 0; i < 2; ++i) {
            _faction[i]->clearOrders();
            for (AI::WarSceneOrder* order : _orders[i]) {
                _faction[i]->addOrder(order);
            }
        }
    } else {
        stringImpl info(
            "Can't reload the navigation mesh this soon.\n Please wait \\[ ");
        info.append(
            Util::toString(Time::MicrosecondsToSeconds(diffTime)).c_str());
        info.append(" ] seconds more!");

        _infoBox->setMessage(info);
        _infoBox->setMessageType(GUIMessageBox::MESSAGE_WARNING);
        _infoBox->show();
    }
    AI::AIManager::getInstance().pauseUpdate(false);
}

void WarScene::processInput(const U64 deltaTime) {}

void WarScene::updateSceneStateInternal(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }
    static U64 totalTime = 0;

    totalTime += deltaTime;

/*if(_lampLightNode && _bobNodeBody){
    static mat4<F32> position =
_lampLightNode->getComponent<PhysicsComponent>()->getWorldMatrix();
    const mat4<F32>& fingerPosition =
_bobNodeBody->getAnimationComponent()->getBoneTransform("fingerstip.R");
    mat4<F32> finalTransform(fingerPosition * position);
    _lampLightNode->getComponent<PhysicsComponent>()->setTransforms(finalTransform.getTranspose());
}*/

#ifdef _DEBUG
    if (!AI::AIManager::getInstance().getNavMesh(
            _army[0][0]->getAgentRadiusCategory())) {
        return;
    }

    _lines[DEBUG_LINE_OBJECT_TO_TARGET].resize(_army[0].size() +
                                               _army[1].size());
    // renderState().drawDebugLines(true);
    U32 count = 0;
    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* character : _army[i]) {
            _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._startPoint.set(
                character->getPosition());
            _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._endPoint.set(
                character->getDestination());
            _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._color.set(
                i == 1 ? 255 : 0, 0, i == 1 ? 0 : 255, 255);
            count++;
        }
    }
#endif
}

bool WarScene::load(const stringImpl& name, GUI* const gui) {
    navMeshStarted = false;
    // Load scene resources
    bool loadState = SCENE_LOAD(name, gui, true, true);
    // Add a light
    _sun = addLight(LIGHT_TYPE_DIRECTIONAL)->getNode<DirectionalLight>();
    // Add a skybox
    _currentSky =
        addSky(CreateResource<Sky>(ResourceDescriptor("Default Sky")));
    // Position camera
    renderState().getCamera().setEye(vec3<F32>(54.5f, 25.5f, 1.5f));
    renderState().getCamera().setGlobalRotation(-90 /*yaw*/, 35 /*pitch*/);
    _sun->csmSplitCount(3);  // 3 splits
    _sun->csmSplitLogFactor(0.925f);
    _sun->csmNearClipOffset(25.0f);
    // Add some obstacles
    SceneGraphNode* cylinder[5];
    cylinder[0] = _sceneGraph.findNode("cylinderC");
    cylinder[1] = _sceneGraph.findNode("cylinderNW");
    cylinder[2] = _sceneGraph.findNode("cylinderNE");
    cylinder[3] = _sceneGraph.findNode("cylinderSW");
    cylinder[4] = _sceneGraph.findNode("cylinderSE");

    for (U8 i = 0; i < 5; ++i) {
        RenderingComponent* const renderable =
            std::begin(cylinder[i]->getChildren())
                ->second->getComponent<RenderingComponent>();
        renderable->getMaterialInstance()->setDoubleSided(true);
        std::begin(cylinder[i]->getChildren())
            ->second->getNode()
            ->getMaterialTpl()
            ->setDoubleSided(true);
    }

    SceneNode* cylinderMeshNW = cylinder[1]->getNode();
    SceneNode* cylinderMeshNE = cylinder[2]->getNode();
    SceneNode* cylinderMeshSW = cylinder[3]->getNode();
    SceneNode* cylinderMeshSE = cylinder[4]->getNode();

    stringImpl currentName;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode* baseNode = nullptr;
    SceneGraphNode* currentNode = nullptr;
    std::pair<I32, I32> currentPos;
    for (U8 i = 0; i < 40; ++i) {
        if (i < 10) {
            baseNode = cylinder[1];
            currentMesh = cylinderMeshNW;
            currentName = "Cylinder_NW_" + Util::toString((I32)i);
            currentPos.first = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        } else if (i >= 10 && i < 20) {
            baseNode = cylinder[2];
            currentMesh = cylinderMeshNE;
            currentName = "Cylinder_NE_" + Util::toString((I32)i);
            currentPos.first = 200 - 40 * (i % 10) - 50;
            currentPos.second = -200 + 40 * (i % 10) + 50;
        } else if (i >= 20 && i < 30) {
            baseNode = cylinder[3];
            currentMesh = cylinderMeshSW;
            currentName = "Cylinder_SW_" + Util::toString((I32)i);
            currentPos.first = -200 + 40 * (i % 20) + 50;
            currentPos.second = 200 - 40 * (i % 20) - 50;
        } else {
            baseNode = cylinder[4];
            currentMesh = cylinderMeshSE;
            currentName = "Cylinder_SE_" + Util::toString((I32)i);
            currentPos.first = 200 - 40 * (i % 30) - 50;
            currentPos.second = 200 - 40 * (i % 30) - 50;
        }

        currentNode = _sceneGraph.getRoot()->addNode(currentMesh, currentName);
        assert(currentNode);
        currentNode->setSelectable(true);
        currentNode->usageContext(baseNode->usageContext());
        PhysicsComponent* pComp = currentNode->getComponent<PhysicsComponent>();
        NavigationComponent* nComp =
            currentNode->getComponent<NavigationComponent>();
        pComp->physicsGroup(
            baseNode->getComponent<PhysicsComponent>()->physicsGroup());
        nComp->navigationContext(
            baseNode->getComponent<NavigationComponent>()->navigationContext());
        nComp->navigationDetailOverride(
            baseNode->getComponent<NavigationComponent>()
                ->navMeshDetailOverride());

        pComp->setScale(baseNode->getComponent<PhysicsComponent>()->getScale());
        pComp->setPosition(
            vec3<F32>(currentPos.first, -0.01f, currentPos.second));
    }
    SceneGraphNode* baseFlagNode = cylinder[1];
    _flag[0] = _sceneGraph.getRoot()->addNode(cylinderMeshNW, "Team1Flag");
    _flag[0]->setSelectable(false);
    _flag[0]->usageContext(baseFlagNode->usageContext());
    PhysicsComponent* flagPComp = _flag[0]->getComponent<PhysicsComponent>();
    NavigationComponent* flagNComp =
        _flag[0]->getComponent<NavigationComponent>();
    flagPComp->physicsGroup(
        baseFlagNode->getComponent<PhysicsComponent>()->physicsGroup());
    flagNComp->navigationContext(NavigationComponent::NODE_IGNORE);
    flagPComp->setScale(
        baseFlagNode->getComponent<PhysicsComponent>()->getScale() *
        vec3<F32>(0.05f, 1.1f, 0.05f));
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, -206.0f));

    _flag[1] = _sceneGraph.getRoot()->addNode(cylinderMeshNW, "Team2Flag");
    _flag[1]->setSelectable(false);
    _flag[1]->usageContext(baseFlagNode->usageContext());

    flagPComp = _flag[1]->getComponent<PhysicsComponent>();
    flagNComp = _flag[1]->getComponent<NavigationComponent>();

    flagPComp->physicsGroup(
        baseFlagNode->getComponent<PhysicsComponent>()->physicsGroup());
    flagNComp->navigationContext(NavigationComponent::NODE_IGNORE);
    flagPComp->setScale(
        baseFlagNode->getComponent<PhysicsComponent>()->getScale() *
        vec3<F32>(0.05f, 1.1f, 0.05f));
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, 206.0f));

    AI::WarSceneAISceneImpl::registerFlags(_flag[0], _flag[1]);

/*_bobNode = _sceneGraph.findNode("Soldier3");
_bobNodeBody = _sceneGraph.findNode("Soldier3_Bob.md5mesh-submesh-0");
_lampLightNode = nullptr;
if(_bobNodeBody != nullptr){
    ResourceDescriptor tempLight("Light_lamp");
    tempLight.setEnumValue(LIGHT_TYPE_POINT);
    // Create a point light for Bob's lamp
    /*Light* light = CreateResource<Light>(tempLight);
    light->setDrawImpostor(true);
    // Make it small and yellow
    light->setCastShadows(false);
    light->setRange(2.0f);
    light->setDiffuseColor(vec3<F32>(1.0f, 0.5f, 0.0f));
    _lampTransform = MemoryManager_NEW SceneTransform();
    // Add it to Bob's body
    _lampTransformNode = _bobNodeBody->addNode(_lampTransform, "lampTransform");
    _lampLightNode = addLight(light, _lampTransformNode);
    // Move it to the lamp's position
    _lampTransformNode->getComponent<PhysicsComponent>()->setPosition(vec3<F32>(-75.0f,
-45.0f, -5.0f));
}*/
//------------------------ The rest of the scene elements
//-----------------------------///

#ifdef _DEBUG
    const U32 particleCount = 200;
#else
    const U32 particleCount = 20000;
#endif

    const F32 emitRate = particleCount / 4;

    ParticleData particles(
        particleCount,
        ParticleData::PROPERTIES_POS | ParticleData::PROPERTIES_VEL |
            ParticleData::PROPERTIES_ACC | ParticleData::PROPERTIES_COLOR |
            ParticleData::PROPERTIES_COLOR_TRANS);

    std::shared_ptr<ParticleSource> particleSource =
        std::make_shared<ParticleSource>(emitRate);
    std::shared_ptr<ParticleBoxGenerator> boxGenerator =
        std::make_shared<ParticleBoxGenerator>();
    particleSource->addGenerator(boxGenerator);
    std::shared_ptr<ParticleColorGenerator> colGenerator =
        std::make_shared<ParticleColorGenerator>();
    particleSource->addGenerator(colGenerator);
    std::shared_ptr<ParticleVelocityGenerator> velGenerator =
        std::make_shared<ParticleVelocityGenerator>();
    particleSource->addGenerator(velGenerator);
    std::shared_ptr<ParticleTimeGenerator> timeGenerator =
        std::make_shared<ParticleTimeGenerator>();
    particleSource->addGenerator(timeGenerator);

    SceneGraphNode* testSGN =
        addParticleEmitter("TESTPARTICLES", particles, _sceneGraph.getRoot());
    ParticleEmitter* test = testSGN->getNode<ParticleEmitter>();
    testSGN->getComponent<PhysicsComponent>()->translateY(5);
    test->setDrawImpostor(true);
    test->enableEmitter(true);
    test->addSource(particleSource);

    std::shared_ptr<ParticleEulerUpdater> eulerUpdater =
        std::make_shared<ParticleEulerUpdater>();
    eulerUpdater->_globalAcceleration.set(0.0f, -15.0f, 0.0f, 0.0f);
    test->addUpdater(eulerUpdater);
    test->addUpdater(std::make_shared<ParticleBasicTimeUpdater>());
    test->addUpdater(std::make_shared<ParticleBasicColorUpdater>());
    test->addUpdater(std::make_shared<ParticleFloorUpdater>());

    state().getGeneralVisibility() *= 2;
    Application::getInstance()
        .getKernel()
        .getCameraMgr()
        .getActiveCamera()
        ->setHorizontalFoV(135);

    _sceneReady = true;
    return loadState;
}

bool WarScene::initializeAI(bool continueOnErrors) {
    //----------------------------Artificial
    //Intelligence------------------------------//
    //   _GOAPContext.setLogLevel(AI::GOAPContext::LOG_LEVEL_NONE);

    // Create 2 AI teams
    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = MemoryManager_NEW AI::AITeam(i);
    }
    // Make the teams fight each other
    _faction[0]->addEnemyTeam(_faction[1]->getTeamID());
    _faction[1]->addEnemyTeam(_faction[0]->getTeamID());

    SceneGraphNode* soldierNode1 = _sceneGraph.findNode("Soldier1");
    SceneGraphNode* soldierNode2 = _sceneGraph.findNode("Soldier2");
    SceneGraphNode* soldierNode3 = _sceneGraph.findNode("Soldier3");
    SceneNode* soldierMesh1 = soldierNode1->getNode();
    SceneNode* soldierMesh2 = soldierNode2->getNode();
    SceneNode* soldierMesh3 = soldierNode3->getNode();
    assert(soldierMesh1 && soldierMesh2 && soldierMesh3);

    vec3<F32> currentScale;
    NPC* soldier = nullptr;
    stringImpl currentName;
    AI::AIEntity* aiSoldier = nullptr;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode* currentNode = nullptr;

    AI::ApproachFlag approachEnemyFlag("ApproachEnemyFlag");
    approachEnemyFlag.setPrecondition(AI::GOAPFact(AI::AtEnemyFlagLoc),
                                      AI::GOAPValue(false));
    approachEnemyFlag.setPrecondition(AI::GOAPFact(AI::HasEnemyFlag),
                                      AI::GOAPValue(false));
    approachEnemyFlag.setEffect(AI::GOAPFact(AI::AtEnemyFlagLoc),
                                AI::GOAPValue(true));
    approachEnemyFlag.setEffect(AI::GOAPFact(AI::AtHomeFlagLoc),
                                AI::GOAPValue(false));

    AI::CaptureFlag captureEnemyFlag("CaptureEnemyFlag");
    captureEnemyFlag.setPrecondition(AI::GOAPFact(AI::AtEnemyFlagLoc),
                                     AI::GOAPValue(true));
    captureEnemyFlag.setPrecondition(AI::GOAPFact(AI::HasEnemyFlag),
                                     AI::GOAPValue(false));
    captureEnemyFlag.setEffect(AI::GOAPFact(AI::HasEnemyFlag),
                               AI::GOAPValue(true));

    AI::ReturnHome returnHome("ReturnHome");
    returnHome.setPrecondition(AI::GOAPFact(AI::AtHomeFlagLoc),
                               AI::GOAPValue(false));
    returnHome.setEffect(AI::GOAPFact(AI::AtHomeFlagLoc), AI::GOAPValue(true));
    returnHome.setEffect(AI::GOAPFact(AI::AtEnemyFlagLoc),
                         AI::GOAPValue(false));

    AI::ReturnFlag returnEnemyFlag("ReturnEnemyFlag");
    returnEnemyFlag.setPrecondition(AI::GOAPFact(AI::AtHomeFlagLoc),
                                    AI::GOAPValue(true));
    returnEnemyFlag.setPrecondition(AI::GOAPFact(AI::HasEnemyFlag),
                                    AI::GOAPValue(true));
    returnEnemyFlag.setEffect(AI::GOAPFact(AI::HasEnemyFlag),
                              AI::GOAPValue(false));

    AI::RecoverFlag recoverFlag("RecoverOwnFlag");
    recoverFlag.setPrecondition(AI::GOAPFact(AI::EnemyDead),
                                AI::GOAPValue(true));
    recoverFlag.setPrecondition(AI::GOAPFact(AI::EnemyHasFlag),
                                AI::GOAPValue(false));
    recoverFlag.setEffect(AI::GOAPFact(AI::HasOwnFlag), AI::GOAPValue(true));

    AI::KillEnemy killEnemy("KillEnemy");
    recoverFlag.setPrecondition(AI::GOAPFact(AI::EnemyDead),
                                AI::GOAPValue(false));
    recoverFlag.setEffect(AI::GOAPFact(AI::EnemyDead), AI::GOAPValue(true));

    AI::GOAPGoal findFlag("Find enemy flag");
    findFlag.setVariable(AI::GOAPFact(AI::AtEnemyFlagLoc), AI::GOAPValue(true));

    AI::GOAPGoal captureFlag("Capture enemy flag");
    captureFlag.setVariable(AI::GOAPFact(AI::HasEnemyFlag),
                            AI::GOAPValue(true));

    AI::GOAPGoal returnFlag("Return enemy flag");
    captureFlag.setVariable(AI::GOAPFact(AI::HasEnemyFlag),
                            AI::GOAPValue(true));
    captureFlag.setVariable(AI::GOAPFact(AI::AtHomeFlagLoc),
                            AI::GOAPValue(true));

    AI::GOAPGoal retrieveFlag("Retrieve flag");
    retrieveFlag.setVariable(AI::GOAPFact(AI::HasOwnFlag), AI::GOAPValue(true));
    retrieveFlag.setVariable(AI::GOAPFact(AI::AtHomeFlagLoc),
                             AI::GOAPValue(true));

    for (U8 k = 0; k < 2; ++k) {
        for (U8 i = 0; i < /*15*/ 1; ++i) {
            F32 speed = 5.5f;  // 5.5 m/s
            U8 zFactor = 0;
            if (i < 5) {
                currentMesh = soldierMesh1;
                currentScale =
                    soldierNode1->getComponent<PhysicsComponent>()->getScale();
                currentName = "Soldier_1_" + Util::toString((I32)k) + "_" +
                              Util::toString((I32)i);
            } else if (i >= 5 && i < 10) {
                currentMesh = soldierMesh2;
                currentScale =
                    soldierNode2->getComponent<PhysicsComponent>()->getScale();
                currentName = "Soldier_2_" + Util::toString((I32)k) + "_" +
                              Util::toString((I32)i % 5);
                speed = 5.75f;
                zFactor = 1;
            } else {
                currentMesh = soldierMesh3;
                currentScale =
                    soldierNode3->getComponent<PhysicsComponent>()->getScale();
                currentName = "Soldier_3_" + Util::toString((I32)k) + "_" +
                              Util::toString((I32)i % 10);
                speed = 5.35f;
                zFactor = 2;
            }

            currentNode =
                _sceneGraph.getRoot()->addNode(currentMesh, currentName);
            currentNode->getComponent<PhysicsComponent>()->setScale(
                currentScale);
            DIVIDE_ASSERT(currentNode != nullptr,
                          "WarScene error: INVALID SOLDIER NODE TEMPLATE!");
            currentNode->setSelectable(true);
            I8 side = k == 0 ? -1 : 1;

            currentNode->getComponent<PhysicsComponent>()->setPosition(
                vec3<F32>(-125 + 25 * (i % 5), -0.01f,
                          200 * side + 25 * zFactor * side));
            if (side == 1) {
                currentNode->getComponent<PhysicsComponent>()->rotateY(180);
                currentNode->getComponent<PhysicsComponent>()->translateX(100);
            }

            currentNode->getComponent<PhysicsComponent>()->translateX(25 *
                                                                      side);

            aiSoldier = MemoryManager_NEW AI::AIEntity(
                currentNode->getComponent<PhysicsComponent>()->getPosition(),
                currentNode->getName());
            aiSoldier->addSensor(AI::VISUAL_SENSOR);
            k == 0
                ? currentNode->getComponent<RenderingComponent>()
                      ->renderBoundingBox(true)
                : currentNode->getComponent<RenderingComponent>()
                      ->renderSkeleton(true);

            AI::WarSceneAISceneImpl* brain =
                MemoryManager_NEW AI::WarSceneAISceneImpl();

            // GOAP
            brain->worldState().setVariable(AI::GOAPFact(AI::AtHomeFlagLoc),
                                            AI::GOAPValue(true));
            brain->worldState().setVariable(AI::GOAPFact(AI::AtEnemyFlagLoc),
                                            AI::GOAPValue(false));
            brain->worldState().setVariable(AI::GOAPFact(AI::HasEnemyFlag),
                                            AI::GOAPValue(false));
            brain->worldState().setVariable(AI::GOAPFact(AI::EnemyDead),
                                            AI::GOAPValue(false));
            brain->worldState().setVariable(AI::GOAPFact(AI::EnemyHasFlag),
                                            AI::GOAPValue(false));
            brain->worldState().setVariable(AI::GOAPFact(AI::HasOwnFlag),
                                            AI::GOAPValue(false));
            brain->worldState().setVariable(AI::GOAPFact(AI::FlagCarrierDead),
                                            AI::GOAPValue(false));

            brain->registerAction(&approachEnemyFlag);
            brain->registerAction(&captureEnemyFlag);
            brain->registerAction(&returnHome);
            brain->registerAction(&returnEnemyFlag);
            brain->registerAction(&recoverFlag);
            brain->registerAction(&killEnemy);
            brain->registerGoal(captureFlag);
            brain->registerGoal(findFlag);
            brain->registerGoal(returnFlag);
            brain->registerGoal(retrieveFlag);

            aiSoldier->addAISceneImpl(brain);
            soldier = MemoryManager_NEW NPC(currentNode, aiSoldier);
            soldier->setMovementSpeed(speed * 2);
            _armyNPCs[k].push_back(soldier);
            _army[k].push_back(aiSoldier);
        }
    }

    //----------------------- AI controlled units ---------------------//
    for (U8 i = 0; i < 2; ++i) {
        for (U8 j = 0; j < _army[i].size(); ++j) {
            AI::AIManager::getInstance().registerEntity(i, _army[i][j]);
        }
    }
    bool state = !(_army[0].empty() || _army[1].empty());

    if (state || continueOnErrors) {
        Scene::initializeAI(continueOnErrors);
    }
    _sceneGraph.getRoot()->deleteNode(soldierNode1);
    _sceneGraph.getRoot()->deleteNode(soldierNode2);
    _sceneGraph.getRoot()->deleteNode(soldierNode3);

    for (U8 i = 0; i < 2; ++i) {
        _orders[i].push_back(MemoryManager_NEW AI::WarSceneOrder(
            AI::WarSceneOrder::ORDER_FIND_ENEMY_FLAG));
        _orders[i].push_back(MemoryManager_NEW AI::WarSceneOrder(
            AI::WarSceneOrder::ORDER_CAPTURE_ENEMY_FLAG));
        _orders[i].push_back(MemoryManager_NEW AI::WarSceneOrder(
            AI::WarSceneOrder::ORDER_RETURN_ENEMY_FLAG));
        _orders[i].push_back(MemoryManager_NEW AI::WarSceneOrder(
            AI::WarSceneOrder::ORDER_PROTECT_FLAG_CARRIER));
        _orders[i].push_back(MemoryManager_NEW AI::WarSceneOrder(
            AI::WarSceneOrder::ORDER_RETRIEVE_FLAG));
    }

    return state;
}

bool WarScene::deinitializeAI(bool continueOnErrors) {
    AI::AIManager::getInstance().pauseUpdate(true);

    while (AI::AIManager::getInstance().updating()) {
    }
    for (U8 i = 0; i < 2; ++i) {
        for (Divide::AI::AIEntity* entity : _army[i]) {
            AI::AIManager::getInstance().unregisterEntity(entity);
        }

        MemoryManager::DELETE_VECTOR(_armyNPCs[i]);
        MemoryManager::DELETE_VECTOR(_army[i]);
        MemoryManager::DELETE_VECTOR(_orders[i]);
        MemoryManager::DELETE(_faction[i]);
    }

    return Scene::deinitializeAI(continueOnErrors);
}

bool WarScene::unload() { return Scene::unload(); }

bool WarScene::loadResources(bool continueOnErrors) {
    _GUI->addButton("Simulate", "Simulate",
                    vec2<I32>(renderState().cachedResolution().width - 220,
                              renderState().cachedResolution().height / 1.1f),
                    vec2<U32>(100, 25), vec3<F32>(0.65f),
                    DELEGATE_BIND(&WarScene::startSimulation, this));

    _GUI->addText("fpsDisplay",  // Unique ID
                  vec2<I32>(60, 60),  // Position
                  Font::DIVIDE_DEFAULT,  // Font
                  vec3<F32>(0.0f, 0.2f, 1.0f),  // Color
                  "FPS: %s", 0);  // Text and arguments
    _GUI->addText("RenderBinCount", vec2<I32>(60, 70), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f, 0.2f, 0.2f),
                  "Number of items in Render Bin: %d", 0);

    _GUI->addText("camPosition", vec2<I32>(60, 100), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | "
                  "Yaw: %5.2f]",
                  renderState().getCamera().getEye().x,
                  renderState().getCamera().getEye().y,
                  renderState().getCamera().getEye().z,
                  renderState().getCamera().getEuler().pitch,
                  renderState().getCamera().getEuler().yaw);

    _GUI->addText("lampPosition", vec2<I32>(60, 120), Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f, 0.8f, 0.2f),
                  "Lamp Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]", 0.0f,
                  0.0f, 0.0f);
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
    _taskTimers.push_back(0.0);  // animation bull
    _taskTimers.push_back(0.0);  // animation dwarf
    _taskTimers.push_back(0.0);  // animation bob
    return true;
}

bool WarScene::onKeyUp(const Input::KeyEvent& key) {
    static bool fpsCameraActive = false;
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    bool keyState = Scene::onKeyUp(key);
    switch (key._key) {
        default:
            break;

        case Input::KeyCode::KC_TAB: {
            if (_currentSelection != nullptr) {
                /*if(flyCameraActive){
                    renderState().getCameraMgr().pushActiveCamera("fpsCamera");
                    flyCameraActive = false; fpsCameraActive = true;
                }else if(fpsCameraActive){*/
                if (flyCameraActive) {
                    if (fpsCameraActive)
                        renderState().getCameraMgr().popActiveCamera();
                    renderState().getCameraMgr().pushActiveCamera("tpsCamera");
                    static_cast<ThirdPersonCamera&>(renderState().getCamera())
                        .setTarget(_currentSelection);
                    /*fpsCameraActive*/ flyCameraActive = false;
                    tpsCameraActive = true;
                    break;
                }
            }
            if (tpsCameraActive) {
                renderState().getCameraMgr().pushActiveCamera("defaultCamera");
                tpsCameraActive = false;
                flyCameraActive = true;
            }
            //            renderState().getCamera().setTargetNode(_currentSelection);
        } break;
    }
    return keyState;
}

bool WarScene::mouseMoved(const Input::MouseEvent& key) {
    return Scene::mouseMoved(key);
}

bool WarScene::mouseButtonPressed(const Input::MouseEvent& key,
                                  Input::MouseButton button) {
    return Scene::mouseButtonPressed(key, button);
}

bool WarScene::mouseButtonReleased(const Input::MouseEvent& key,
                                   Input::MouseButton button) {
    return Scene::mouseButtonReleased(key, button);
    ;
}
};
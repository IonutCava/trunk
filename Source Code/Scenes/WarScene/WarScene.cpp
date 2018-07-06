#include "stdafx.h"

#include "Headers/WarScene.h"
#include "Headers/WarSceneAIProcessor.h"

#include "GUI/Headers/GUIMessageBox.h"
#include "Geometry/Material/Headers/Material.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Math/Headers/Transform.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicTimeUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleBasicColourUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleEulerUpdater.h"
#include "Dynamics/Entities/Particles/ConcreteUpdaters/Headers/ParticleFloorUpdater.h"

#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleBoxGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleColourGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleVelocityGenerator.h"
#include "Dynamics/Entities/Particles/ConcreteGenerators/Headers/ParticleTimeGenerator.h"

namespace Divide {

namespace {
    vec2<F32> g_sunAngle(0.0f, Angle::to_RADIANS(45.0f));
    bool g_direction = false;
    U64 elapsedGameTimeUs = 0;
};

WarScene::WarScene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name)
   : Scene(context, cache, parent, name),
    _infoBox(nullptr),
    _sceneReady(false),
    _lastNavMeshBuildTime(0UL)
{

    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = nullptr;
    }

    _resetUnits = false;

    addSelectionCallback([&](U8 playerIndex) {
        if (!_currentSelection[playerIndex].expired()) {
            _GUI->modifyText(_ID("entityState"), _currentSelection[playerIndex].lock()->getName().c_str());
        } else {
            _GUI->modifyText(_ID("entityState"), "");
        }
    });

    _targetLines = _context.gfx().newIMP();

    _runCount = 0;
    _timeLimitMinutes = 5;
    _scoreLimit = 3;
    _elapsedGameTime = 0;
}

WarScene::~WarScene()
{
	_targetLines->clear();
}

void WarScene::processGUI(const U64 deltaTime) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimers[0] >= FpsDisplay) {
        const Camera& cam = _scenePlayers.front()->getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime()));
        _GUI->modifyText(_ID("RenderBinCount"),
            Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                               _context.gfx().parent().renderPassManager().getLastTotalBinSize(RenderStage::DISPLAY),
                               _context.gfx().getLastCullCount()));

        _GUI->modifyText(_ID("camPosition"),
                         Util::StringFormat("Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
                                            eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));

        _guiTimers[0] = 0.0;
    }

    if (_guiTimers[1] >= Time::SecondsToMilliseconds(1)) {
        if (!_currentSelection[0].expired()) {
            AI::AIEntity* entity = findAI(_currentSelection[0].lock());
            if (entity) {
                _GUI->modifyText(_ID("entityState"), entity->toString().c_str());
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

        _GUI->modifyText(_ID("scoreDisplay"),
            Util::StringFormat("Score: A -  %d B - %d [Limit: %d]\nElapsed game time [ %d:%d:%d / %d:%d:%d]",
                               AI::WarSceneAIProcessor::getScore(0),
                               AI::WarSceneAIProcessor::getScore(1),
                               _scoreLimit,
                               elapsedTimeMinutes,
                               elapsedTimeSeconds,
                               elapsedTimeMilliseconds,
                               limitTimeMinutes,
                               limitTimeSeconds,
                               limitTimeMilliseconds));

        _guiTimers[2] = 0.0;
    }

    Scene::processGUI(deltaTime);
}

namespace {
    F32 phiLight = 0.0f;
    vec3<F32> initPosLight[16];
    vec3<F32> initPosLight2[80];
    
    bool initPosSetLight = false;
    vec2<F32> rotatePoint(vec2<F32> center, F32 angle, vec2<F32> point) {

        return vec2<F32>(std::cos(angle) * (point.x - center.x) - std::sin(angle) * (point.y - center.y) + center.x,
                         std::sin(angle) * (point.x - center.x) + std::cos(angle) * (point.y - center.y) + center.y);
    }
};

void WarScene::debugDraw(const Camera& activeCamera, const RenderStagePass& stagePass, RenderSubPassCmds& subPassesInOut) {
    subPassesInOut.back()._commands.push_back(_targetLines->toDrawCommand());
    Scene::debugDraw(activeCamera, stagePass, subPassesInOut);
}

void WarScene::processTasks(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

    D64 SunTimer = Time::Milliseconds(33);
    D64 AnimationTimer1 = Time::SecondsToMilliseconds(5);
    D64 AnimationTimer2 = Time::SecondsToMilliseconds(10);
    D64 updateLights = Time::Milliseconds(16);

    if (_taskTimers[0] >= SunTimer) {
        g_sunAngle += 0.000125f * (g_direction ? 1.0f : -1.0f);

        if (!IS_IN_RANGE_INCLUSIVE(g_sunAngle.y, 
                                   Angle::DEGREES<F32>(20.0f),
                                   Angle::DEGREES<F32>(55.0f))) {
            g_direction = !g_direction;
        }

        vec3<F32> sunVector(-cosf(g_sunAngle.x) * sinf(g_sunAngle.y),
                            -cosf(g_sunAngle.y),
                            -sinf(g_sunAngle.x) * sinf(g_sunAngle.y));

        _sun.lock()->get<PhysicsComponent>()->setPosition(sunVector);
        vec4<F32> sunColour = vec4<F32>(1.0f, 1.0f, 0.2f, 1.0f);

        _sun.lock()->getNode<Light>()->setDiffuseColour(sunColour);
        _currentSky.lock()->getNode<Sky>()->setSunProperties(sunVector, _sun.lock()->getNode<Light>()->getDiffuseColour());

        _taskTimers[0] = 0.0;
    }

    if (_taskTimers[1] >= AnimationTimer1) {
        for (SceneGraphNode_wptr npc : _armyNPCs[0]) {
            assert(npc.lock());
            npc.lock()->get<UnitComponent>()->getUnit<NPC>()->playNextAnimation();
        }
        _taskTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= AnimationTimer2) {
        for (SceneGraphNode_wptr npc : _armyNPCs[1]) {
            assert(npc.lock());
            npc.lock()->get<UnitComponent>()->getUnit<NPC>()->playNextAnimation();
        }
        _taskTimers[2] = 0.0;
    }

    if (!initPosSetLight) {
        for (U8 i = 0; i < 16; ++i) {
            initPosLight[i].set(_lightNodes[i].lock()->get<PhysicsComponent>()->getPosition());
        }
        for (U8 i = 0; i < 80; ++i) {
            initPosLight2[i].set(_lightNodes2[i].first.lock()->get<PhysicsComponent>()->getPosition());
        }
        initPosSetLight = true;
    }

    if (_taskTimers[3] >= updateLights) {

        phiLight += 0.01f;
        if (phiLight > 360.0f) {
            phiLight = 0.0f;
        }
        F32 s1 = std::sin(phiLight);
        F32 c1 = std::cos(phiLight);
        F32 s2 = std::sin(-phiLight);
        F32 c2 = std::cos(-phiLight);
        const F32 radius = 10;
        for (U8 i = 0; i < 16; ++i) {
            F32 c = i % 2 == 0 ? c1 : c2;
            F32 s = i % 2 == 0 ? s1 : s2;
            SceneGraphNode_ptr light = _lightNodes[i].lock();
            PhysicsComponent* pComp = light->get<PhysicsComponent>();
            pComp->setPositionX(radius * c + initPosLight[i].x);
            pComp->setPositionZ(radius * s + initPosLight[i].z);
            pComp->setPositionY((radius * 0.5f) * s + initPosLight[i].y);
        }

        for (U8 i = 0; i < 80; ++i) {
            SceneGraphNode_ptr light = _lightNodes2[i].first.lock();
            PhysicsComponent* pComp = light->get<PhysicsComponent>();
            F32 angle = _lightNodes2[i].second ? std::fmod(phiLight + 45.0f, 360.0f) : phiLight;
            vec2<F32> position(rotatePoint(vec2<F32>(0.0f), angle, initPosLight2[i].xz()));
            pComp->setPositionX(position.x);
            pComp->setPositionZ(position.y);
        }

        for (U8 i = 0; i < 40; ++i) {
            SceneGraphNode_ptr light = _lightNodes3[i].lock();
            PhysicsComponent* pComp = light->get<PhysicsComponent>();
            pComp->rotateY(phiLight);
        }
        _taskTimers[3] = 0.0;
    }

    Scene::processTasks(deltaTime);
}

namespace {
    F32 phi = 0.0f;
    vec3<F32> initPos;
    bool initPosSet = false;
};

void WarScene::updateSceneStateInternal(const U64 deltaTime) {
    if (!_sceneReady) {
        return;
    }

    if (_resetUnits) {
        resetUnits();
        _resetUnits = false;
    }

    _targetLines->paused(!renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_TARGET_LINES));

    SceneGraphNode_ptr particles = _particleEmitter.lock();
    const F32 radius = 20;
    
    if (particles.get()) {
        phi += 0.01f;
        if (phi > 360.0f) {
            phi = 0.0f;
        }

        PhysicsComponent* pComp = particles->get<PhysicsComponent>();
        if (!initPosSet) {
            initPos.set(pComp->getPosition());
            initPosSet = true;
        }

        /*pComp->setPositionX(radius * std::cos(phi) + initPos.x);
        pComp->setPositionZ(radius * std::sin(phi) + initPos.z);
        pComp->setPositionY((radius * 0.5f) * std::sin(phi) + initPos.y);
        pComp->rotateY(phi);*/
    }

    if (!_aiManager->getNavMesh(_armyNPCs[0][0].lock()->get<UnitComponent>()->getUnit<NPC>()->getAIEntity()->getAgentRadiusCategory())) {
        return;
    }

    // renderState().drawDebugLines(true);
    vec3<F32> tempDestination;
    vec4<U8> redLine(255,0,0,128);
    vec4<U8> blueLine(0,0,255,128);
    vectorImpl<Line> paths;
    paths.reserve(_armyNPCs[0].size() + _armyNPCs[1].size());
    for (U8 i = 0; i < 2; ++i) {
        for (SceneGraphNode_wptr node : _armyNPCs[i]) {
            AI::AIEntity* const character = node.lock()->get<UnitComponent>()->getUnit<NPC>()->getAIEntity();
            if (!node.lock()->isActive()) {
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
    _targetLines->fromLines(paths);

    if (!_aiManager->updatePaused()) {
        _elapsedGameTime += deltaTime;
        checkGameCompletion();
    }
}

bool WarScene::load(const stringImpl& name) {
    static const U32 lightMask = to_base(SGNComponent::ComponentType::PHYSICS) |
                                 to_base(SGNComponent::ComponentType::BOUNDS) |
                                 to_base(SGNComponent::ComponentType::RENDERING);

    static const U32 normalMask = lightMask |
                                  to_base(SGNComponent::ComponentType::NAVIGATION) |
                                  to_base(SGNComponent::ComponentType::NETWORKING);

    // Load scene resources
    bool loadState = SCENE_LOAD(name, true, true);
    // Add a light
    _sun = addLight(LightType::DIRECTIONAL, _sceneGraph->getRoot());
    // Add a skybox
    _currentSky = addSky();
    // Position camera
    _baseCamera->setEye(vec3<F32>(43.13f, 147.09f, -4.41f));
    _baseCamera->setGlobalRotation(-90.0f /*yaw*/, 59.21f /*pitch*/);

    _sun.lock()->getNode<DirectionalLight>()->csmSplitCount(3);  // 3 splits
    _sun.lock()->getNode<DirectionalLight>()->csmSplitLogFactor(0.85f);
    _sun.lock()->getNode<DirectionalLight>()->csmNearClipOffset(25.0f);
    // Add some obstacles
    SceneGraphNode_ptr cylinder[5];
    cylinder[0] = _sceneGraph->findNode("cylinderC").lock();
    cylinder[1] = _sceneGraph->findNode("cylinderNW").lock();
    cylinder[2] = _sceneGraph->findNode("cylinderNE").lock();
    cylinder[3] = _sceneGraph->findNode("cylinderSW").lock();
    cylinder[4] = _sceneGraph->findNode("cylinderSE").lock();

    for (U8 i = 0; i < 5; ++i) {
        RenderingComponent* const renderable = cylinder[i]->getChild(0).get<RenderingComponent>();
        renderable->getMaterialInstance()->setDoubleSided(true);
        cylinder[i]->getChild(0).getNode()->getMaterialTpl()->setDoubleSided(true);
    }

    // Make the center cylinder reflective
    const Material_ptr& matInstance = cylinder[0]->getChild(0).get<RenderingComponent>()->getMaterialInstance();
    matInstance->setShininess(200);

    std::shared_ptr<SceneNode> cylinderMeshNW = cylinder[1]->getNode();
    std::shared_ptr<SceneNode> cylinderMeshNE = cylinder[2]->getNode();
    std::shared_ptr<SceneNode> cylinderMeshSW = cylinder[3]->getNode();
    std::shared_ptr<SceneNode> cylinderMeshSE = cylinder[4]->getNode();

    stringImpl currentName;
    std::shared_ptr<SceneNode> currentMesh;
    SceneGraphNode_ptr baseNode;

    U8 locationFlag = 0;
    std::pair<I32, I32> currentPos;
    for (U8 i = 0; i < 40; ++i) {
        if (i < 10) {
            baseNode = cylinder[1];
            currentMesh = cylinderMeshNW;
            currentName = "Cylinder_NW_" + to_stringImpl((I32)i);
            currentPos.first = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        }
        else if (i >= 10 && i < 20) {
            baseNode = cylinder[2];
            currentMesh = cylinderMeshNE;
            currentName = "Cylinder_NE_" + to_stringImpl((I32)i);
            currentPos.first = 200 - 40 * (i % 10) - 50;
            currentPos.second = -200 + 40 * (i % 10) + 50;
            locationFlag = 1;
        }
        else if (i >= 20 && i < 30) {
            baseNode = cylinder[3];
            currentMesh = cylinderMeshSW;
            currentName = "Cylinder_SW_" + to_stringImpl((I32)i);
            currentPos.first = -200 + 40 * (i % 20) + 50;
            currentPos.second = 200 - 40 * (i % 20) - 50;
            locationFlag = 2;
        }
        else {
            baseNode = cylinder[4];
            currentMesh = cylinderMeshSE;
            currentName = "Cylinder_SE_" + to_stringImpl((I32)i);
            currentPos.first = 200 - 40 * (i % 30) - 50;
            currentPos.second = 200 - 40 * (i % 30) - 50;
            locationFlag = 3;
        }

        SceneGraphNode_ptr crtNode = _sceneGraph->getRoot().addNode(currentMesh, normalMask, baseNode->get<PhysicsComponent>()->physicsGroup(), currentName);
        crtNode->setSelectable(true);
        crtNode->usageContext(baseNode->usageContext());
        PhysicsComponent* pComp = crtNode->get<PhysicsComponent>();
        NavigationComponent* nComp = crtNode->get<NavigationComponent>();
        nComp->navigationContext(
            baseNode->get<NavigationComponent>()->navigationContext());
        nComp->navigationDetailOverride(
            baseNode->get<NavigationComponent>()
            ->navMeshDetailOverride());

        vec3<F32> position(to_F32(currentPos.first), -0.01f, to_F32(currentPos.second));
        pComp->setScale(baseNode->get<PhysicsComponent>()->getScale());
        pComp->setPosition(position);

        {
            ResourceDescriptor tempLight(Util::StringFormat("Light_point_%s_1", currentName.c_str()));
            tempLight.setEnumValue(to_base(LightType::POINT));
            tempLight.setUserPtr(_lightPool);
            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(false);
            light->setRange(25.0f);
            //light->setCastShadows(i == 0 ? true : false);
            light->setCastShadows(false);
            light->setDiffuseColour(DefaultColours::RANDOM());
            SceneGraphNode_ptr lightSGN = _sceneGraph->getRoot().addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
            lightSGN->get<PhysicsComponent>()->setPosition(position + vec3<F32>(0.0f, 8.0f, 0.0f));
            _lightNodes2.push_back(std::make_pair(lightSGN, false));
        }
        {
            ResourceDescriptor tempLight(Util::StringFormat("Light_point_%s_2", currentName.c_str()));
            tempLight.setEnumValue(to_base(LightType::POINT));
            tempLight.setUserPtr(_lightPool);
            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(false);
            light->setRange(35.0f);
            light->setCastShadows(false);
            light->setDiffuseColour(DefaultColours::RANDOM());
            SceneGraphNode_ptr lightSGN = _sceneGraph->getRoot().addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
            lightSGN->get<PhysicsComponent>()->setPosition(position + vec3<F32>(0.0f, 8.0f, 0.0f));
            _lightNodes2.push_back(std::make_pair(lightSGN, true));
        }
        {
            ResourceDescriptor tempLight(Util::StringFormat("Light_spot_%s", currentName.c_str()));
            tempLight.setEnumValue(to_base(LightType::SPOT));
            tempLight.setUserPtr(_lightPool);
            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(false);
            light->setRange(55.0f);
            //light->setCastShadows(i == 1 ? true : false);
            light->setCastShadows(false);
            light->setDiffuseColour(DefaultColours::RANDOM());
            SceneGraphNode_ptr lightSGN = _sceneGraph->getRoot().addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
            lightSGN->get<PhysicsComponent>()->setPosition(position + vec3<F32>(0.0f, 10.0f, 0.0f));
            lightSGN->get<PhysicsComponent>()->rotateX(-20);
            _lightNodes3.push_back(lightSGN);
        }
    }

    SceneGraphNode_ptr flag;
    flag = _sceneGraph->findNode("flag").lock();
    RenderingComponent* const renderable = flag->getChild(0).get<RenderingComponent>();
    renderable->getMaterialInstance()->setDoubleSided(true);
    const Material_ptr& mat = flag->getChild(0).getNode()->getMaterialTpl();
    mat->setDoubleSided(true);
    flag->setActive(false);
    std::shared_ptr<SceneNode> flagNode = flag->getNode();

    _flag[0] = _sceneGraph->getRoot().addNode(flagNode, normalMask, flag->get<PhysicsComponent>()->physicsGroup(), "Team1Flag");

    SceneGraphNode_ptr flag0(_flag[0].lock());
    flag0->setSelectable(false);
    flag0->usageContext(flag->usageContext());
    PhysicsComponent* flagPComp = flag0->get<PhysicsComponent>();
    NavigationComponent* flagNComp = flag0->get<NavigationComponent>();
    RenderingComponent* flagRComp = flag0->getChild(0).get<RenderingComponent>();

    flagPComp->setScale(flag->get<PhysicsComponent>()->getScale());
    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, -206.0f));

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::BLUE());

    _flag[1] = _sceneGraph->getRoot().addNode(flagNode, normalMask, flag->get<PhysicsComponent>()->physicsGroup(), "Team2Flag");
    SceneGraphNode_ptr flag1(_flag[1].lock());
    flag1->setSelectable(false);
    flag1->usageContext(flag->usageContext());

    flagPComp = flag1->get<PhysicsComponent>();
    flagNComp = flag1->get<NavigationComponent>();
    flagRComp = flag1->getChild(0).get<RenderingComponent>();

    flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, 206.0f));
    flagPComp->setScale(flag->get<PhysicsComponent>()->getScale());

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::RED());

    SceneGraphNode_ptr firstPersonFlag = _sceneGraph->getRoot().addNode(flagNode, normalMask, PhysicsGroup::GROUP_KINEMATIC, "FirstPersonFlag");
    firstPersonFlag->lockVisibility(true);
    firstPersonFlag->onCollisionCbk(DELEGATE_BIND(&WarScene::weaponCollision, this, std::placeholders::_1));
    firstPersonFlag->usageContext(SceneGraphNode::UsageContext::NODE_DYNAMIC);
    flagPComp = firstPersonFlag->get<PhysicsComponent>();
    flagPComp->setScale(0.0015f);
    flagPComp->setPosition(1.25f, -1.5f, 0.15f);
    flagPComp->rotate(-20.0f, -70.0f, 50.0f);
    flagRComp = firstPersonFlag->getChild(0).get<RenderingComponent>();
    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::GREEN());
    flagRComp->getMaterialInstance()->setHighPriority(true);
    _firstPersonWeapon = firstPersonFlag;

    //_firstPersonWeapon.lock()->get<PhysicsComponent>()->ignoreView(true, Camera::activeCamera()->getGUID());

    AI::WarSceneAIProcessor::registerFlags(_flag[0], _flag[1]);

    AI::WarSceneAIProcessor::registerScoreCallback([&](U8 teamID, const stringImpl& unitName) {
        registerPoint(teamID, unitName);
    });

    AI::WarSceneAIProcessor::registerMessageCallback([&](U8 eventID, const stringImpl& unitName) {
        printMessage(eventID, unitName);
    });
    
    const U32 particleCount = Config::Build::IS_DEBUG_BUILD ? 4000 : 20000;
    const F32 emitRate = particleCount / 4;

    std::shared_ptr<ParticleData> particles = 
        std::make_shared<ParticleData>(particleCount,
                                       to_base(ParticleData::Properties::PROPERTIES_POS) |
                                       to_base(ParticleData::Properties::PROPERTIES_VEL) |
                                       to_base(ParticleData::Properties::PROPERTIES_ACC) |
                                       to_base(ParticleData::Properties::PROPERTIES_COLOR) |
                                       to_base(ParticleData::Properties::PROPERTIES_COLOR_TRANS));
    particles->_textureFileName = "particle.DDS";

    std::shared_ptr<ParticleSource> particleSource =  std::make_shared<ParticleSource>(emitRate);

    std::shared_ptr<ParticleBoxGenerator> boxGenerator = std::make_shared<ParticleBoxGenerator>();
    boxGenerator->maxStartPosOffset(vec4<F32>(0.3f, 0.0f, 0.3f, 1.0f));
    particleSource->addGenerator(boxGenerator);

    std::shared_ptr<ParticleColourGenerator> colGenerator = std::make_shared<ParticleColourGenerator>();
    colGenerator->_minStartCol.set(Util::ToByteColour(vec4<F32>(0.7f, 0.4f, 0.4f, 1.0f)));
    colGenerator->_maxStartCol.set(Util::ToByteColour(vec4<F32>(1.0f, 0.8f, 0.8f, 1.0f)));
    colGenerator->_minEndCol.set(Util::ToByteColour(vec4<F32>(0.5f, 0.2f, 0.2f, 0.5f)));
    colGenerator->_maxEndCol.set(Util::ToByteColour(vec4<F32>(0.7f, 0.5f, 0.5f, 0.75f)));
    particleSource->addGenerator(colGenerator);

    std::shared_ptr<ParticleVelocityGenerator> velGenerator = std::make_shared<ParticleVelocityGenerator>();
    velGenerator->_minStartVel.set(-1.0f, 0.22f, -1.0f, 0.0f);
    velGenerator->_maxStartVel.set(1.0f, 3.45f, 1.0f, 0.0f);
    particleSource->addGenerator(velGenerator);

    std::shared_ptr<ParticleTimeGenerator> timeGenerator = std::make_shared<ParticleTimeGenerator>();
    timeGenerator->_minTime = 8.5f;
    timeGenerator->_maxTime = 20.5f;
    particleSource->addGenerator(timeGenerator);

    _particleEmitter = addParticleEmitter("TESTPARTICLES", particles, _sceneGraph->getRoot());
    SceneGraphNode_ptr testSGN = _particleEmitter.lock();
    std::shared_ptr<ParticleEmitter> test = testSGN->getNode<ParticleEmitter>();
    testSGN->get<PhysicsComponent>()->translateY(10);
    test->setDrawImpostor(true);
    test->enableEmitter(true);
    test->addSource(particleSource);
    boxGenerator->pos(vec4<F32>(testSGN->get<PhysicsComponent>()->getPosition()));

    std::shared_ptr<ParticleEulerUpdater> eulerUpdater = std::make_shared<ParticleEulerUpdater>();
    eulerUpdater->_globalAcceleration.set(0.0f, -20.0f, 0.0f);
    test->addUpdater(eulerUpdater);
    std::shared_ptr<ParticleFloorUpdater> floorUpdater = std::make_shared<ParticleFloorUpdater>();
    floorUpdater->_bounceFactor = 0.65f;
    test->addUpdater(floorUpdater);
    test->addUpdater(std::make_shared<ParticleBasicTimeUpdater>());
    test->addUpdater(std::make_shared<ParticleBasicColourUpdater>());
    
    state().renderState().generalVisibility(state().renderState().generalVisibility() * 2);


    for (U8 row = 0; row < 4; row++) {
        for (U8 col = 0; col < 4; col++) {
            ResourceDescriptor tempLight(Util::StringFormat("Light_point_%d_%d", row, col));
            tempLight.setEnumValue(to_base(LightType::POINT));
            tempLight.setUserPtr(_lightPool);
            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(false);
            light->setRange(20.0f);
            light->setCastShadows(false);
            light->setDiffuseColour(DefaultColours::RANDOM());
            SceneGraphNode_ptr lightSGN = _sceneGraph->getRoot().addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
            lightSGN->get<PhysicsComponent>()->setPosition(vec3<F32>(-215.0f + (115 * row), 15.0f, (-215.0f + (115 * col))));
            _lightNodes.push_back(lightSGN);
        }
    }

    _baseCamera->setHorizontalFoV(110);

    _envProbePool->addInfiniteProbe(vec3<F32>(0.0f, 0.0f, 0.0f));
    _envProbePool->addLocalProbe(vec3<F32>(-5.0f), vec3<F32>(-1.0f));

    _sceneReady = true;
    return loadState;
}

U16 WarScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;
    _input->actionList().registerInputAction(actionID, DELEGATE_BIND(&WarScene::toggleCamera, this));
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_TAB, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, DELEGATE_BIND(&WarScene::registerPoint, this, to_U16(0), ""));
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_1, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, DELEGATE_BIND(&WarScene::registerPoint, this, to_U16(1), ""));
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_2, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [](InputParams param) {DIVIDE_ASSERT(false, "Test Assert"); });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_5, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](InputParams param) {
        /// TTT -> TTF -> TFF -> FFT -> FTT -> TFT -> TTT
        bool dir   = _lightPool->lightTypeEnabled(LightType::DIRECTIONAL);
        bool point = _lightPool->lightTypeEnabled(LightType::POINT);
        bool spot  = _lightPool->lightTypeEnabled(LightType::SPOT);
        if (dir && point && spot) {
            _lightPool->toggleLightType(LightType::SPOT, false);
        } else if (dir && point && !spot) {
            _lightPool->toggleLightType(LightType::POINT, false);
        } else if (dir && !point && !spot) {
            _lightPool->toggleLightType(LightType::DIRECTIONAL, false);
            _lightPool->toggleLightType(LightType::SPOT, true);
        } else if (!dir && !point && spot) {
            _lightPool->toggleLightType(LightType::POINT, true);
        } else if (!dir && point && spot) {
            _lightPool->toggleLightType(LightType::DIRECTIONAL, true);
            _lightPool->toggleLightType(LightType::POINT, false);
        } else {
            _lightPool->toggleLightType(LightType::POINT, true);
        }
    });
    actions._onReleaseAction = actionID;
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);


    return actionID++;
}

void WarScene::toggleCamera() {
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    if (!_currentSelection[0].expired()) {
        if (flyCameraActive) {
            Camera::activeCamera(_ID("tpsCamera"));
            static_cast<ThirdPersonCamera&>(*Camera::activeCamera()).setTarget(_currentSelection[0]);
            flyCameraActive = false;
            tpsCameraActive = true;
            return;
        }
    }
    if (tpsCameraActive) {
        Camera::activeCamera(&_scenePlayers[_parent.playerPass()]->getCamera());
        tpsCameraActive = false;
        flyCameraActive = true;
    }
}

bool WarScene::loadResources(bool continueOnErrors) {
    // Add a first person camera
    Camera* cam = Camera::createCamera("fpsCamera", Camera::CameraType::FIRST_PERSON);
    cam->fromCamera(*_baseCamera);
    cam->setMoveSpeedFactor(10.0f);
    cam->setTurnSpeedFactor(10.0f);
    // Add a third person camera
    cam = Camera::createCamera("tpsCamera", Camera::CameraType::THIRD_PERSON);
    cam->fromCamera(*_baseCamera);
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);

    _guiTimers.push_back(0.0);  // Fps
    _guiTimers.push_back(0.0);  // AI info
    _guiTimers.push_back(0.0);  // Game info
    _taskTimers.push_back(0.0); // Sun animation
    _taskTimers.push_back(0.0); // animation team 1
    _taskTimers.push_back(0.0); // animation team 2
    _taskTimers.push_back(0.0); // light timer
    return true;
}

void WarScene::postLoadMainThread() {
    const vec2<U16>& resolution = _GUI->getDisplayResolution();

    _GUI->addButton(_ID("Simulate"), "Simulate",
        vec2<I32>(resolution.width - 220, 60),
        vec2<U32>(100, 25),
        DELEGATE_BIND(&WarScene::startSimulation, this, std::placeholders::_1));

    _GUI->addButton(_ID("ShaderReload"), "Shader Reload",
        vec2<I32>(resolution.width - 220, 30),
        vec2<U32>(100, 25),
        [this](I64 btnID) { rebuildShaders(); });

    _GUI->addText(_ID("fpsDisplay"),  // Unique ID
        vec2<I32>(60, 63),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec4<U8>(0, 50, 255, 255), // Colour
        Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f", 0.0f, 0.0f));  // Text and arguments
    _GUI->addText(_ID("RenderBinCount"),
        vec2<I32>(60, 83),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(164, 50, 50, 255),
        Util::StringFormat("Number of items in Render Bin: %d", 0));
    _GUI->addText(_ID("camPosition"), vec2<I32>(60, 103),
        Font::DIVIDE_DEFAULT,
        vec4<U8>(50, 192, 50, 255),
        Util::StringFormat("Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f));

    _GUI->addText(_ID("scoreDisplay"),
        vec2<I32>(60, 123),  // Position
        Font::DIVIDE_DEFAULT,  // Font
        vec4<U8>(50, 192, 50, 255),// Colour
        Util::StringFormat("Score: A -  %d B - %d", 0, 0));  // Text and arguments

    _GUI->addText(_ID("entityState"),
                  vec2<I32>(60, 163),
                  Font::DIVIDE_DEFAULT,
                  vec4<U8>(0, 0, 0, 255),
                  "");

    _infoBox = _GUI->addMsgBox(_ID("infoBox"), "Info", "Blabla");

    Scene::postLoadMainThread();
}

void WarScene::weaponCollision(SceneGraphNode_cptr collider) {
    if (collider) {
        Console::d_printfn("Weapon touched [ %s ]", collider->getName().c_str());
    }
}

};

#include "stdafx.h"

#include "Headers/WarScene.h"
#include "Headers/WarSceneAIProcessor.h"

#include "GUI/Headers/GUIButton.h"
#include "GUI/Headers/GUIMessageBox.h"
#include "Geometry/Material/Headers/Material.h"
#include "Core/Headers/Kernel.h"
#include "Core/Headers/PlatformContext.h"
#include "Core/Time/Headers/ApplicationTimer.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/SubMesh.h"
#include "Geometry/Shapes/Predefined/Headers/Sphere3D.h"
#include "Geometry/Shapes/Predefined/Headers/Quad3D.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Dynamics/Entities/Units/Headers/Player.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/RenderPassManager.h"
#include "Platform/Video/Headers/IMPrimitive.h"

#include "ECS/Components/Headers/BoundsComponent.h"
#include "ECS/Components/Headers/UnitComponent.h"
#include "ECS/Components/Headers/RigidBodyComponent.h"
#include "ECS/Components/Headers/RenderingComponent.h"
#include "ECS/Components/Headers/TransformComponent.h"
#include "ECS/Components/Headers/DirectionalLightComponent.h"

#include "Environment/Terrain/Headers/Terrain.h"

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
    _terrainMode(false),
    _lastNavMeshBuildTime(0UL),
    _firstPersonWeapon(nullptr)
{

    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = nullptr;
    }

    _resetUnits = false;

    addSelectionCallback([&](PlayerIndex idx, SceneGraphNode* node) {
        if (node != nullptr) {
            _GUI->modifyText(_ID("entityState"), node->name().c_str());
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

void WarScene::processGUI(const U64 deltaTimeUS) {
    D64 FpsDisplay = Time::SecondsToMilliseconds(0.3);
    if (_guiTimersMS[0] >= FpsDisplay) {
        const Camera& cam = _scenePlayers.front()->getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        _GUI->modifyText(_ID("fpsDisplay"),
                         Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f. FrameIndex : %d",
                                            Time::ApplicationTimer::instance().getFps(),
                                            Time::ApplicationTimer::instance().getFrameTime(),
                                            _context.gfx().getFrameCount()));
        _GUI->modifyText(_ID("RenderBinCount"),
            Util::StringFormat("Number of items in Render Bin: %d. Number of HiZ culled items: %d",
                               _context.gfx().parent().renderPassManager().getLastTotalBinSize(RenderStage::DISPLAY),
                               _context.gfx().getLastCullCount()));

        _GUI->modifyText(_ID("camPosition"),
                         Util::StringFormat("Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
                                            eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw));

        _guiTimersMS[0] = 0.0;
    }

    if (_guiTimersMS[1] >= Time::SecondsToMilliseconds(1)) {
        if (!_currentSelection[0].empty()) {
            SceneGraphNode* node = sceneGraph().findNode(_currentSelection[0].front());
            if (node != nullptr) {
                AI::AIEntity* entity = findAI(node);
                if (entity) {
                    _GUI->modifyText(_ID("entityState"), entity->toString().c_str());
                }
            }
        }
    }

    if (_guiTimersMS[2] >= 66) {
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

        _guiTimersMS[2] = 0.0;
    }

    Scene::processGUI(deltaTimeUS);
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

void WarScene::toggleTerrainMode() {
    _terrainMode = !_terrainMode;
}

void WarScene::debugDraw(const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
    if (renderState().isEnabledOption(SceneRenderState::RenderOptions::RENDER_DEBUG_TARGET_LINES)) {
        bufferInOut.add(_targetLines->toCommandBuffer());
    }
    Scene::debugDraw(activeCamera, stagePass, bufferInOut);
}

void WarScene::processTasks(const U64 deltaTimeUS) {
    return;

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

        _sun->get<TransformComponent>()->setRotationEuler(sunVector);
        FColour sunColour = FColour(1.0f, 1.0f, 0.2f, 1.0f);

        _sun->get<DirectionalLightComponent>()->setDiffuseColour(sunColour);

        PushConstants& constants = _currentSky->get<RenderingComponent>()->pushConstants();
        constants.set("enable_sun", GFX::PushConstantType::BOOL, true);
        constants.set("sun_vector", GFX::PushConstantType::VEC3, sunVector);
        constants.set("sun_colour", GFX::PushConstantType::VEC3, sunColour);

        _taskTimers[0] = 0.0;
    }

    if (_taskTimers[1] >= AnimationTimer1) {
        for (SceneGraphNode* npc : _armyNPCs[0]) {
            assert(npc);
            npc->get<UnitComponent>()->getUnit<NPC>()->playNextAnimation();
        }
        _taskTimers[1] = 0.0;
    }

    if (_taskTimers[2] >= AnimationTimer2) {
        for (SceneGraphNode* npc : _armyNPCs[1]) {
            assert(npc);
            npc->get<UnitComponent>()->getUnit<NPC>()->playNextAnimation();
        }
        _taskTimers[2] = 0.0;
    }

    if (!initPosSetLight) {
        for (U8 i = 0; i < 16; ++i) {
            initPosLight[i].set(_lightNodes[i]->get<TransformComponent>()->getPosition());
        }
        for (U8 i = 0; i < 80; ++i) {
            initPosLight2[i].set(_lightNodes2[i].first->get<TransformComponent>()->getPosition());
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
            SceneGraphNode* light = _lightNodes[i];
            TransformComponent* tComp = light->get<TransformComponent>();
            tComp->setPositionX(radius * c + initPosLight[i].x);
            tComp->setPositionZ(radius * s + initPosLight[i].z);
            tComp->setPositionY((radius * 0.5f) * s + initPosLight[i].y);
        }

        for (U8 i = 0; i < 80; ++i) {
            SceneGraphNode* light = _lightNodes2[i].first;
            TransformComponent* tComp = light->get<TransformComponent>();
            F32 angle = _lightNodes2[i].second ? std::fmod(phiLight + 45.0f, 360.0f) : phiLight;
            vec2<F32> position(rotatePoint(vec2<F32>(0.0f), angle, initPosLight2[i].xz()));
            tComp->setPositionX(position.x);
            tComp->setPositionZ(position.y);
        }

        for (U8 i = 0; i < 40; ++i) {
            SceneGraphNode* light = _lightNodes3[i];
            TransformComponent* tComp = light->get<TransformComponent>();
            tComp->rotateY(phiLight);
        }
        _taskTimers[3] = 0.0;
    }

    Scene::processTasks(deltaTimeUS);
}

namespace {
    F32 phi = 0.0f;
    vec3<F32> initPos;
    bool initPosSet = false;
};

namespace{
    SceneGraphNode* g_terrain = nullptr;
};

void WarScene::updateSceneStateInternal(const U64 deltaTimeUS) {
    if (!_sceneReady) {
        return;
    }


    if (_terrainMode) {
        if (g_terrain == nullptr) {
            auto objects = sceneGraph().getNodesByType(SceneNodeType::TYPE_OBJECT3D);
            for (SceneGraphNode* object : objects) {
                if (object->getNode<Object3D>().getObjectType()._value == ObjectType::TERRAIN) {
                    g_terrain = object;
                    break;
                }
            }
        } else {
            vec3<F32> camPos = playerCamera()->getEye();
            if (g_terrain->get<BoundsComponent>()->getBoundingBox().containsPoint(camPos)) {   
                const Terrain& ter = g_terrain->getNode<Terrain>();

                F32 headHeight = state().playerState(state().playerPass()).headHeight();
                camPos -= g_terrain->get<TransformComponent>()->getPosition();
                playerCamera()->setEye(ter.getPositionFromGlobal(camPos.x, camPos.z, true) + vec3<F32>(0.0f, headHeight, 0.0f));
            }
        }
    }

    return;

    if (_resetUnits) {
        resetUnits();
        _resetUnits = false;
    }

    SceneGraphNode* particles = _particleEmitter;
    const F32 radius = 20;
    
    if (particles) {
        phi += 0.01f;
        if (phi > 360.0f) {
            phi = 0.0f;
        }

        TransformComponent* tComp = particles->get<TransformComponent>();
        if (!initPosSet) {
            initPos.set(tComp->getPosition());
            initPosSet = true;
        }

        /*tComp->setPositionX(radius * std::cos(phi) + initPos.x);
        tComp->setPositionZ(radius * std::sin(phi) + initPos.z);
        tComp->setPositionY((radius * 0.5f) * std::sin(phi) + initPos.y);
        tComp->rotateY(phi);*/
    }
    


    if (!_aiManager->getNavMesh(_armyNPCs[0][0]->get<UnitComponent>()->getUnit<NPC>()->getAIEntity()->getAgentRadiusCategory())) {
        return;
    }
    
    // renderState().drawDebugLines(true);
    vec3<F32> tempDestination;
    UColour redLine(255,0,0,128);
    UColour blueLine(0,0,255,128);
    vector<Line> paths;
    paths.reserve(_armyNPCs[0].size() + _armyNPCs[1].size());
    for (U8 i = 0; i < 2; ++i) {
        for (SceneGraphNode* node : _armyNPCs[i]) {
            AI::AIEntity* const character = node->get<UnitComponent>()->getUnit<NPC>()->getAIEntity();
            if (!node->isActive()) {
                continue;
            }
            tempDestination.set(character->getDestination());
            if (!tempDestination.isZeroLength()) {
                paths.emplace_back(
                    character->getPosition(),
                    tempDestination,
                    i == 0 ? blueLine : redLine,
                    i == 0 ? blueLine / 2 : redLine / 2);
            }
        }
    }
    _targetLines->fromLines(paths);

    if (!_aiManager->updatePaused()) {
        _elapsedGameTime += deltaTimeUS;
        checkGameCompletion();
    }
}

bool WarScene::load(const stringImpl& name) {
    static const U32 lightMask = to_base(ComponentType::TRANSFORM) |
                                 to_base(ComponentType::BOUNDS) |
                                 to_base(ComponentType::RENDERING);

    static const U32 normalMask = lightMask |
                                  to_base(ComponentType::RIGID_BODY) |
                                  to_base(ComponentType::NAVIGATION) |
                                  to_base(ComponentType::NETWORKING);

    // Load scene resources
    bool loadState = SCENE_LOAD(name);
    // Position camera
    Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->setEye(vec3<F32>(43.13f, 147.09f, -4.41f));
    Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->setGlobalRotation(-90.0f /*yaw*/, 59.21f /*pitch*/);

    //_sun->get<DirectionalLightComponent>()->csmSplitCount(3);  // 3 splits
    _sun->get<DirectionalLightComponent>()->csmSplitLogFactor(0.85f);
    _sun->get<DirectionalLightComponent>()->csmNearClipOffset(25.0f);
    // Add some obstacles

    SceneGraphNode* cylinder[5];
    cylinder[0] = _sceneGraph->findNode("cylinderC");
    cylinder[1] = _sceneGraph->findNode("cylinderNW");
    cylinder[2] = _sceneGraph->findNode("cylinderNE");
    cylinder[3] = _sceneGraph->findNode("cylinderSW");
    cylinder[4] = _sceneGraph->findNode("cylinderSE");

    for (U8 i = 0; i < 5; ++i) {
        RenderingComponent* const renderable = cylinder[i]->getChild(0).get<RenderingComponent>();
        renderable->getMaterialInstance()->setDoubleSided(true);
        cylinder[i]->getChild(0).getNode().getMaterialTpl()->setDoubleSided(true);
    }

    // Make the center cylinder reflective
    const Material_ptr& matInstance = cylinder[0]->getChild(0).get<RenderingComponent>()->getMaterialInstance();
    matInstance->setShininess(200);

#if 0
    SceneNode_ptr cylinderMeshNW = cylinder[1]->getNode();
    SceneNode_ptr cylinderMeshNE = cylinder[2]->getNode();
    SceneNode_ptr cylinderMeshSW = cylinder[3]->getNode();
    SceneNode_ptr cylinderMeshSE = cylinder[4]->getNode();

    stringImpl currentName;
    SceneNode_ptr currentMesh;
    SceneGraphNode* baseNode;

    SceneGraphNodeDescriptor sceneryNodeDescriptor;
    sceneryNodeDescriptor._serialize = false;
    sceneryNodeDescriptor._componentMask = normalMask;

    U8 locationFlag = 0;
    std::pair<I32, I32> currentPos;
    for (U8 i = 0; i < 40; ++i) {
        if (i < 10) {
            baseNode = cylinder[1];
            currentMesh = cylinderMeshNW;
            currentName = "Cylinder_NW_" + to_stringImpl((I32)i);
            currentPos.first = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        } else if (i >= 10 && i < 20) {
            baseNode = cylinder[2];
            currentMesh = cylinderMeshNE;
            currentName = "Cylinder_NE_" + to_stringImpl((I32)i);
            currentPos.first = 200 - 40 * (i % 10) - 50;
            currentPos.second = -200 + 40 * (i % 10) + 50;
            locationFlag = 1;
        } else if (i >= 20 && i < 30) {
            baseNode = cylinder[3];
            currentMesh = cylinderMeshSW;
            currentName = "Cylinder_SW_" + to_stringImpl((I32)i);
            currentPos.first = -200 + 40 * (i % 20) + 50;
            currentPos.second = 200 - 40 * (i % 20) - 50;
            locationFlag = 2;
        } else {
            baseNode = cylinder[4];
            currentMesh = cylinderMeshSE;
            currentName = "Cylinder_SE_" + to_stringImpl((I32)i);
            currentPos.first = 200 - 40 * (i % 30) - 50;
            currentPos.second = 200 - 40 * (i % 30) - 50;
            locationFlag = 3;
        }


        
        sceneryNodeDescriptor._node = currentMesh;
        sceneryNodeDescriptor._name = currentName;
        sceneryNodeDescriptor._usageContext = baseNode->usageContext();
        sceneryNodeDescriptor._physicsGroup = baseNode->get<RigidBodyComponent>()->physicsGroup();

        SceneGraphNode* crtNode = _sceneGraph->getRoot().addNode(sceneryNodeDescriptor);
        
        TransformComponent* tComp = crtNode->get<TransformComponent>();
        NavigationComponent* nComp = crtNode->get<NavigationComponent>();
        nComp->navigationContext(baseNode->get<NavigationComponent>()->navigationContext());
        nComp->navigationDetailOverride(baseNode->get<NavigationComponent>()->navMeshDetailOverride());

        vec3<F32> position(to_F32(currentPos.first), -0.01f, to_F32(currentPos.second));
        tComp->setScale(baseNode->get<TransformComponent>()->getScale());
        tComp->setPosition(position);

        /*{
            ResourceDescriptor tempLight(Util::StringFormat("Light_point_%s_1", currentName.c_str()));
            tempLight.setEnumValue(to_base(LightType::POINT));
            tempLight.setUserPtr(_lightPool);
            std::shared_ptr<Light> light = CreateResource<Light>(_resCache, tempLight);
            light->setDrawImpostor(false);
            light->setRange(25.0f);
            //light->setCastShadows(i == 0 ? true : false);
            light->setCastShadows(false);
            light->setDiffuseColour(DefaultColours::RANDOM());
            SceneGraphNode* lightSGN = _sceneGraph->getRoot().addNode(light, lightMask);
            lightSGN->get<TransformComponent>()->setPosition(position + vec3<F32>(0.0f, 8.0f, 0.0f));
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
            SceneGraphNode* lightSGN = _sceneGraph->getRoot().addNode(light, lightMask);
            lightSGN->get<TransformComponent>()->setPosition(position + vec3<F32>(0.0f, 8.0f, 0.0f));
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
            SceneGraphNode* lightSGN = _sceneGraph->getRoot().addNode(light, lightMask);
            lightSGN->get<TransformComponent>()->setPosition(position + vec3<F32>(0.0f, 10.0f, 0.0f));
            lightSGN->get<TransformComponent>()->rotateX(-20);
            _lightNodes3.push_back(lightSGN);
        }*/
    }

    SceneGraphNode* flag;
    flag = _sceneGraph->findNode("flag");
    RenderingComponent* const renderable = flag->getChild(0).get<RenderingComponent>();
    renderable->getMaterialInstance()->setDoubleSided(true);
    const Material_ptr& mat = flag->getChild(0).getNode()->getMaterialTpl();
    mat->setDoubleSided(true);
    flag->setActive(false);
    SceneNode_ptr flagNode = flag->getNode();

    sceneryNodeDescriptor._usageContext = NodeUsageContext::NODE_DYNAMIC;
    sceneryNodeDescriptor._node = flagNode;
    sceneryNodeDescriptor._physicsGroup = flag->get<RigidBodyComponent>()->physicsGroup();
    sceneryNodeDescriptor._name = "Team1Flag";

    _flag[0] = _sceneGraph->getRoot().addNode(sceneryNodeDescriptor);

    SceneGraphNode* flag0(_flag[0]);

    TransformComponent* flagtComp = flag0->get<TransformComponent>();
    NavigationComponent* flagNComp = flag0->get<NavigationComponent>();
    RenderingComponent* flagRComp = flag0->getChild(0).get<RenderingComponent>();

    flagtComp->setScale(flag->get<TransformComponent>()->getScale());
    flagtComp->setPosition(vec3<F32>(25.0f, 0.1f, -206.0f));

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::BLUE);

    sceneryNodeDescriptor._name = "Team2Flag";
    _flag[1] = _sceneGraph->getRoot().addNode(sceneryNodeDescriptor);
    SceneGraphNode* flag1(_flag[1]);
    flag1->usageContext(flag->usageContext());

    flagtComp = flag1->get<TransformComponent>();
    flagNComp = flag1->get<NavigationComponent>();
    flagRComp = flag1->getChild(0).get<RenderingComponent>();

    flagtComp->setPosition(vec3<F32>(25.0f, 0.1f, 206.0f));
    flagtComp->setScale(flag->get<TransformComponent>()->getScale());

    flagNComp->navigationContext(NavigationComponent::NavigationContext::NODE_IGNORE);

    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::RED);

    sceneryNodeDescriptor._name = "FirstPersonFlag";
    SceneGraphNode* firstPersonFlag = _sceneGraph->getRoot().addNode(sceneryNodeDescriptor);
    firstPersonFlag->lockVisibility(true);

    flagtComp = firstPersonFlag->get<TransformComponent>();
    flagtComp->setScale(0.0015f);
    flagtComp->setPosition(1.25f, -1.5f, 0.15f);
    flagtComp->rotate(-20.0f, -70.0f, 50.0f);

    auto collision = [this](const RigidBodyComponent& collider) {
        weaponCollision(collider);
    };
    RigidBodyComponent* rComp = firstPersonFlag->get<RigidBodyComponent>();
    rComp->onCollisionCbk(collision);
    flagRComp = firstPersonFlag->getChild(0).get<RenderingComponent>();
    flagRComp->getMaterialInstance()->setDiffuse(DefaultColours::GREEN);

    firstPersonFlag->get<RigidBodyComponent>()->physicsGroup(PhysicsGroup::GROUP_KINEMATIC);

    _firstPersonWeapon = firstPersonFlag;

    AI::WarSceneAIProcessor::registerFlags(_flag[0], _flag[1]);

    AI::WarSceneAIProcessor::registerScoreCallback([&](U8 teamID, const stringImpl& unitName) {
        registerPoint(teamID, unitName);
    });

    AI::WarSceneAIProcessor::registerMessageCallback([&](U8 eventID, const stringImpl& unitName) {
        printMessage(eventID, unitName);
    });
#endif
    static const bool disableParticles = true;

    if (!disableParticles) {
        const U32 particleCount = Config::Build::IS_DEBUG_BUILD ? 4000 : 20000;
        const F32 emitRate = particleCount / 4;

        std::shared_ptr<ParticleData> particles =
            std::make_shared<ParticleData>(context().gfx(),
                particleCount,
                to_base(ParticleData::Properties::PROPERTIES_POS) |
                to_base(ParticleData::Properties::PROPERTIES_VEL) |
                to_base(ParticleData::Properties::PROPERTIES_ACC) |
                to_base(ParticleData::Properties::PROPERTIES_COLOR) |
                to_base(ParticleData::Properties::PROPERTIES_COLOR_TRANS));
        particles->_textureFileName = "particle.DDS";

        std::shared_ptr<ParticleSource> particleSource = std::make_shared<ParticleSource>(context().gfx(), emitRate);

        std::shared_ptr<ParticleBoxGenerator> boxGenerator(new ParticleBoxGenerator());
        boxGenerator->maxStartPosOffset(vec4<F32>(0.3f, 0.0f, 0.3f, 1.0f));
        particleSource->addGenerator(boxGenerator);

        std::shared_ptr<ParticleColourGenerator> colGenerator(new ParticleColourGenerator());
        colGenerator->_minStartCol.set(Util::ToByteColour(FColour(0.7f, 0.4f, 0.4f, 1.0f)));
        colGenerator->_maxStartCol.set(Util::ToByteColour(FColour(1.0f, 0.8f, 0.8f, 1.0f)));
        colGenerator->_minEndCol.set(Util::ToByteColour(FColour(0.5f, 0.2f, 0.2f, 0.5f)));
        colGenerator->_maxEndCol.set(Util::ToByteColour(FColour(0.7f, 0.5f, 0.5f, 0.75f)));
        particleSource->addGenerator(colGenerator);

        std::shared_ptr<ParticleVelocityGenerator> velGenerator(new ParticleVelocityGenerator());
        velGenerator->_minStartVel.set(-1.0f, 0.22f, -1.0f, 0.0f);
        velGenerator->_maxStartVel.set(1.0f, 3.45f, 1.0f, 0.0f);
        particleSource->addGenerator(velGenerator);

        std::shared_ptr<ParticleTimeGenerator> timeGenerator(new ParticleTimeGenerator());
        timeGenerator->_minTime = 8.5f;
        timeGenerator->_maxTime = 20.5f;
        particleSource->addGenerator(timeGenerator);

        /*_particleEmitter = addParticleEmitter("TESTPARTICLES", particles, _sceneGraph->getRoot());
        SceneGraphNode* testSGN = _particleEmitter;
        std::shared_ptr<ParticleEmitter> test = testSGN->getNode<ParticleEmitter>();
        testSGN->get<TransformComponent>()->translateY(10);
        test->setDrawImpostor(true);
        test->enableEmitter(true);
        test->addSource(particleSource);
        boxGenerator->pos(vec4<F32>(testSGN->get<TransformComponent>()->getPosition()));

        std::shared_ptr<ParticleEulerUpdater> eulerUpdater = std::make_shared<ParticleEulerUpdater>(platformContext().gfx());
        eulerUpdater->_globalAcceleration.set(0.0f, -20.0f, 0.0f);
        test->addUpdater(eulerUpdater);
        std::shared_ptr<ParticleFloorUpdater> floorUpdater = std::make_shared<ParticleFloorUpdater>(platformContext().gfx());
        floorUpdater->_bounceFactor = 0.65f;
        test->addUpdater(floorUpdater);
        test->addUpdater(std::make_shared<ParticleBasicTimeUpdater>(platformContext().gfx()));
        test->addUpdater(std::make_shared<ParticleBasicColourUpdater>(platformContext().gfx()));
        */
    }

    //state().renderState().generalVisibility(state().renderState().generalVisibility() * 2);

    /*
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
            SceneGraphNode* lightSGN = _sceneGraph->getRoot().addNode(light, lightMask, PhysicsGroup::GROUP_IGNORE);
            lightSGN->get<TransformComponent>()->setPosition(vec3<F32>(-215.0f + (115 * row), 15.0f, (-215.0f + (115 * col))));
            _lightNodes.push_back(lightSGN);
        }
    }
    */
    Camera::utilityCamera(Camera::UtilityCamera::DEFAULT)->setHorizontalFoV(110);

    static const bool disableEnvProbes = true;
    if (!disableEnvProbes) {
        _envProbePool->addInfiniteProbe(vec3<F32>(0.0f, 0.0f, 0.0f));
        _envProbePool->addLocalProbe(vec3<F32>(-5.0f), vec3<F32>(-1.0f));
    }

    _sceneReady = true;
    if (loadState) {
        return initializeAI(true);
    }
    return false;
}


bool WarScene::unload() {
    deinitializeAI(true);
    return Scene::unload();
}

U16 WarScene::registerInputActions() {
    U16 actionID = Scene::registerInputActions();

    //ToDo: Move these to per-scene XML file
    PressReleaseActions actions;
    _input->actionList().registerInputAction(actionID, [this](const InputParams& param) {toggleCamera(param); });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_TAB, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](const InputParams& param) {registerPoint(0u, ""); });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_1, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [this](const InputParams& param) {registerPoint(1u, ""); });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_2, actions);
    actionID++;

    _input->actionList().registerInputAction(actionID, [](InputParams param) {DIVIDE_ASSERT(false, "Test Assert"); });
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
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
    actions.actionID(PressReleaseActions::Action::RELEASE, actionID);
    _input->addKeyMapping(Input::KeyCode::KC_L, actions);


    return actionID++;
}

void WarScene::toggleCamera(InputParams param) {
    // None of this works with multiple players
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;
    static Camera* tpsCamera = nullptr;
    static Camera* fpsCamera = nullptr;

    if (!tpsCamera) {
        tpsCamera = Camera::findCamera(_ID("tpsCamera"));
        fpsCamera = Camera::findCamera(_ID("fpsCamera"));
    }

    PlayerIndex idx = getPlayerIndexForDevice(param._deviceIndex);
    if (!_currentSelection[idx].empty()) {
        SceneGraphNode* node = sceneGraph().findNode(_currentSelection[idx].front());
        if (node != nullptr) {
            if (flyCameraActive) {
                state().playerState(idx).overrideCamera(tpsCamera);
                static_cast<ThirdPersonCamera&>(*tpsCamera).setTarget(node);
                flyCameraActive = false;
                tpsCameraActive = true;
                return;
            }
        }
    }
    if (tpsCameraActive) {
        state().playerState(idx).overrideCamera(nullptr);
        tpsCameraActive = false;
        flyCameraActive = true;
    }
}

void WarScene::postLoadMainThread() {
    const vec2<U16>& resolution = _context.gfx().renderingResolution();

    GUIButton* btn = _GUI->addButton(_ID("Simulate"),
                                     "Simulate",
                                     pixelPosition(resolution.width - 220, 60),
                                     pixelScale(100, 25));
    btn->setEventCallback(GUIButton::Event::MouseClick, [this](I64 btnGUID) { startSimulation(btnGUID); });
                            
    btn = _GUI->addButton(_ID("ShaderReload"),
                          "Shader Reload",
                          pixelPosition(resolution.width - 220, 30),
                          pixelScale(100, 25));
    btn->setEventCallback(GUIButton::Event::MouseClick,
                         [this](I64 btnID) { rebuildShaders(); });

    btn = _GUI->addButton(_ID("TerrainMode"),
                               "Terrain Mode Toggle",
                               pixelPosition(resolution.width - 240, 90),
                               pixelScale(120, 25));
    btn->setEventCallback(GUIButton::Event::MouseClick,
        [this](I64 btnID) { toggleTerrainMode(); });


    _GUI->addText("fpsDisplay",  // Unique ID
                  pixelPosition(60, 63),  // Position
        Font::DIVIDE_DEFAULT,  // Font
                  UColour(0, 50, 255, 255), // Colour
        Util::StringFormat("FPS: %3.0f. FrameTime: %3.1f", 0.0f, 0.0f));  // Text and arguments
    _GUI->addText("RenderBinCount",
                  pixelPosition(60, 83),
        Font::DIVIDE_DEFAULT,
                  UColour(164, 50, 50, 255),
        Util::StringFormat("Number of items in Render Bin: %d", 0));
    _GUI->addText("camPosition", pixelPosition(60, 103),
        Font::DIVIDE_DEFAULT,
                  UColour(50, 192, 50, 255),
        Util::StringFormat("Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f));


    _GUI->addText("scoreDisplay",
                  pixelPosition(60, 123),  // Position
        Font::DIVIDE_DEFAULT,  // Font
                  UColour(50, 192, 50, 255),// Colour
        Util::StringFormat("Score: A -  %d B - %d", 0, 0));  // Text and arguments

    _GUI->addText("entityState",
                  pixelPosition(60, 163),
                  Font::DIVIDE_DEFAULT,
                  UColour(0, 0, 0, 255),
                  "");

    _infoBox = _GUI->addMsgBox(_ID("infoBox"), "Info", "Blabla");

    // Add a first person camera
    Camera* cam = Camera::createCamera("fpsCamera", Camera::CameraType::FIRST_PERSON);
    cam->fromCamera(*Camera::utilityCamera(Camera::UtilityCamera::DEFAULT));
    cam->setMoveSpeedFactor(10.0f);
    cam->setTurnSpeedFactor(10.0f);
    // Add a third person camera
    cam = Camera::createCamera("tpsCamera", Camera::CameraType::THIRD_PERSON);
    cam->fromCamera(*Camera::utilityCamera(Camera::UtilityCamera::DEFAULT));
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);

    _guiTimersMS.push_back(0.0);  // Fps
    _guiTimersMS.push_back(0.0);  // AI info
    _guiTimersMS.push_back(0.0);  // Game info
    _taskTimers.push_back(0.0); // Sun animation
    _taskTimers.push_back(0.0); // animation team 1
    _taskTimers.push_back(0.0); // animation team 2
    _taskTimers.push_back(0.0); // light timer

    Scene::postLoadMainThread();
}

void WarScene::onSetActive() {
    Scene::onSetActive();
    //_firstPersonWeapon->lockToCamera(_ID(playerCamera()->name()));
}

void WarScene::weaponCollision(const RigidBodyComponent& collider) {
    Console::d_printfn("Weapon touched [ %s ]", collider.getSGN().name().c_str());
}

};

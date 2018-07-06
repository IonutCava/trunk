#include "Headers/WarScene.h"
#include "Headers/WarSceneAISceneImpl.h"

#include "GUI/Headers/GUIMessageBox.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {

namespace {
    static std::atomic_bool g_navMeshStarted;
};

bool WarScene::initializeAI(bool continueOnErrors) {
    //------------------------Artificial Intelligence------------------------//
    // Create 2 AI teams
    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = MemoryManager_NEW AI::AITeam(i);
    }
    // Make the teams fight each other
    _faction[0]->addEnemyTeam(_faction[1]->getTeamID());
    _faction[1]->addEnemyTeam(_faction[0]->getTeamID());

    SceneGraphNode* lightNode = _sceneGraph.findNode("Soldier1");
    SceneGraphNode* animalNode = _sceneGraph.findNode("Soldier2");
    SceneGraphNode* heavyNode = _sceneGraph.findNode("Soldier3");

    SceneNode* lightNodeMesh = lightNode->getNode();
    SceneNode* animalNodeMesh = animalNode->getNode();
    SceneNode* heavyNodeMesh = heavyNode->getNode();
    assert(lightNodeMesh && animalNodeMesh && heavyNodeMesh);

    NPC* soldier = nullptr;
    AI::AIEntity* aiSoldier = nullptr;
    SceneNode* currentMesh = nullptr;

    vec3<F32> currentScale;
    stringImpl currentName;

    AI::ApproachFlag approachEnemyFlag("ApproachEnemyFlag");
    approachEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::AtEnemyFlagLoc),
                                      AI::GOAPValue(false));
    approachEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                                      AI::GOAPValue(false));
    approachEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::Idling),
                                      AI::GOAPFact(true));
    approachEnemyFlag.setEffect(AI::GOAPFact(AI::Fact::AtEnemyFlagLoc),
                                AI::GOAPValue(true));
    approachEnemyFlag.setEffect(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                                AI::GOAPValue(false));
    approachEnemyFlag.setEffect(AI::GOAPFact(AI::Fact::Idling),
                                AI::GOAPFact(false));

    AI::CaptureFlag captureEnemyFlag("CaptureEnemyFlag");
    captureEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::AtEnemyFlagLoc),
                                     AI::GOAPValue(true));
    captureEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                                     AI::GOAPValue(false));
    captureEnemyFlag.setEffect(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                               AI::GOAPValue(true));

    AI::ReturnFlagHome returnToBase("ReturnFlagToBase");
    returnToBase.setPrecondition(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                                 AI::GOAPValue(false));
    returnToBase.setPrecondition(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                                 AI::GOAPValue(true));
    returnToBase.setEffect(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                           AI::GOAPValue(true));

    AI::ScoreFlag scoreEnemyFlag("ScoreEnemyFlag");
    scoreEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                                   AI::GOAPValue(true));
    scoreEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                                   AI::GOAPValue(true));
    scoreEnemyFlag.setPrecondition(AI::GOAPFact(AI::Fact::EnemyHasFlag),
                                   AI::GOAPValue(false));
    scoreEnemyFlag.setEffect(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                             AI::GOAPValue(false));

    AI::Idle idleAction("Idle");
    idleAction.setPrecondition(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                               AI::GOAPValue(true));
    idleAction.setEffect(AI::GOAPFact(AI::Fact::Idling), AI::GOAPFact(true));

    AI::AttackEnemy attackAction("Attack");
    attackAction.setPrecondition(AI::GOAPFact(AI::Fact::EnemyDead), 
                                 AI::GOAPValue(false));
    attackAction.setEffect(AI::GOAPFact(AI::Fact::EnemyDead), 
                           AI::GOAPValue(true));

    AI::GOAPGoal captureFlag(
        "Capture enemy flag",
        to_uint(AI::WarSceneOrder::WarOrder::ORDER_CAPTURE_ENEMY_FLAG));
    captureFlag.setVariable(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                            AI::GOAPValue(true));
    captureFlag.setVariable(AI::GOAPFact(AI::Fact::AtHomeFlagLoc),
                            AI::GOAPValue(true));

    AI::GOAPGoal scoreFlag(
        "Score", to_uint(AI::WarSceneOrder::WarOrder::ORDER_SCORE_ENEMY_FLAG));
    scoreFlag.setVariable(AI::GOAPFact(AI::Fact::HasEnemyFlag),
                          AI::GOAPValue(false));

    AI::GOAPGoal idle("Idle", to_uint(AI::WarSceneOrder::WarOrder::ORDER_IDLE));
    idle.setVariable(AI::GOAPFact(AI::Fact::Idling), AI::GOAPValue(true));

    AI::GOAPGoal killEnemy("Kill", to_uint(AI::WarSceneOrder::WarOrder::ORDER_KILL_ENEMY));
    killEnemy.setVariable(AI::GOAPFact(AI::Fact::EnemyDead), AI::GOAPValue(true));

    std::array<AI::GOAPPackage,
               to_const_uint(AI::WarSceneAISceneImpl::AIType::COUNT)>
        goapPackages;
    for (AI::GOAPPackage& goapPackage : goapPackages) {
        goapPackage._worldState.setVariable(
            AI::GOAPFact(AI::Fact::AtEnemyFlagLoc), AI::GOAPValue(false));
        goapPackage._worldState.setVariable(
            AI::GOAPFact(AI::Fact::AtHomeFlagLoc), AI::GOAPValue(true));
        goapPackage._worldState.setVariable(
            AI::GOAPFact(AI::Fact::HasEnemyFlag), AI::GOAPValue(false));
        goapPackage._worldState.setVariable(
            AI::GOAPFact(AI::Fact::EnemyHasFlag), AI::GOAPValue(false));
        goapPackage._worldState.setVariable(AI::GOAPFact(AI::Fact::Idling),
                                            AI::GOAPValue(true));
        goapPackage._worldState.setVariable(AI::GOAPFact(AI::Fact::EnemyDead),
                                            AI::GOAPValue(false));

        goapPackage._actionSet.push_back(idleAction);
        goapPackage._actionSet.push_back(attackAction);
        goapPackage._goalList.push_back(idle);
        goapPackage._goalList.push_back(killEnemy);
    }

    AI::GOAPPackage& animalPackage =
        goapPackages[to_uint(AI::WarSceneAISceneImpl::AIType::ANIMAL)];
    AI::GOAPPackage& lightPackage =
        goapPackages[to_uint(AI::WarSceneAISceneImpl::AIType::LIGHT)];
    AI::GOAPPackage& heavyPackage =
        goapPackages[to_uint(AI::WarSceneAISceneImpl::AIType::HEAVY)];

    heavyPackage._actionSet.push_back(approachEnemyFlag);
    heavyPackage._actionSet.push_back(captureEnemyFlag);
    heavyPackage._actionSet.push_back(returnToBase);
    heavyPackage._actionSet.push_back(scoreEnemyFlag);
    heavyPackage._goalList.push_back(captureFlag);
    heavyPackage._goalList.push_back(scoreFlag);

    SceneGraphNode& root = GET_ACTIVE_SCENEGRAPH().getRoot();
    for (I32 k = 0; k < 2; ++k) {
        for (I32 i = 0; i < 15; ++i) {
            F32 speed = 5.5f;  // 5.5 m/s
            F32 zFactor = 0.0f;
            I32 damage = 5;
            AI::WarSceneAISceneImpl::AIType type;
            if (IS_IN_RANGE_INCLUSIVE(i, 0, 4)) {
                currentMesh = lightNodeMesh;
                currentScale =
                    lightNode->getComponent<PhysicsComponent>()->getScale();
                currentName = Util::stringFormat("Soldier_1_%d_%d", k, i);
                type = AI::WarSceneAISceneImpl::AIType::LIGHT;
            } else if (IS_IN_RANGE_INCLUSIVE(i, 5, 9)) {
                currentMesh = animalNodeMesh;
                currentScale =
                    animalNode->getComponent<PhysicsComponent>()->getScale();
                currentName = Util::stringFormat("Soldier_2_%d_%d", k, i % 5);
                speed = 5.75f;
                zFactor = 1.0f;
                type = AI::WarSceneAISceneImpl::AIType::ANIMAL;
                damage = 10;
            } else {
                currentMesh = heavyNodeMesh;
                currentScale =
                    heavyNode->getComponent<PhysicsComponent>()->getScale();
                currentName = Util::stringFormat("Soldier_3_%d_%d", k, i % 10);
                speed = 5.35f;
                zFactor = 2.0f;
                type = AI::WarSceneAISceneImpl::AIType::HEAVY;
                damage = 15;
            }

            SceneGraphNode& currentNode =
                root.addNode(*currentMesh, currentName);
            currentNode.setSelectable(true);

            PhysicsComponent* pComp =
                currentNode.getComponent<PhysicsComponent>();
            pComp->setScale(currentScale);

            if (k == 0) {
                zFactor *= -25;
                zFactor -= 200;
                pComp->translateX(-25);
            } else {
                zFactor *= 25;
                zFactor += 200;
                pComp->rotateY(180);
                pComp->translateX(100);
                pComp->translateX(25);
            }

            pComp->setPosition(vec3<F32>(-125 + 25 * (i % 5), -0.01f, zFactor));

            aiSoldier = MemoryManager_NEW AI::AIEntity(pComp->getPosition(),
                                                       currentNode.getName());
            aiSoldier->addSensor(AI::SensorType::VISUAL_SENSOR);
            k == 0
                ? currentNode.getComponent<RenderingComponent>()
                      ->renderBoundingBox(true)
                : currentNode.getComponent<RenderingComponent>()
                      ->renderSkeleton(true);

            AI::WarSceneAISceneImpl* brain =
                MemoryManager_NEW AI::WarSceneAISceneImpl(type);

            // GOAP
            brain->registerGOAPPackage(goapPackages[to_uint(type)]);
            aiSoldier->addAISceneImpl(brain);
            soldier = MemoryManager_NEW NPC(currentNode, aiSoldier);
            soldier->setAttribute(to_uint(AI::UnitAttributes::HEALTH_POINTS), 100);
            soldier->setAttribute(to_uint(AI::UnitAttributes::DAMAGE), damage);
            soldier->setAttribute(to_uint(AI::UnitAttributes::ALIVE_FLAG), 1);
            
            _army[k].push_back(aiSoldier);
            _armyNPCs[k].push_back(soldier);
            
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

    _sceneGraph.getRoot().deleteNode(lightNode);
    _sceneGraph.getRoot().deleteNode(animalNode);
    _sceneGraph.getRoot().deleteNode(heavyNode);

    return state;
}

bool WarScene::deinitializeAI(bool continueOnErrors) {
    AI::AIManager::getInstance().pauseUpdate(true);

    while (AI::AIManager::getInstance().updating()) {
    }

    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* const entity : _army[i]) {
            AI::AIManager::getInstance().unregisterEntity(entity);
        }

        MemoryManager::DELETE_VECTOR(_army[i]);
        MemoryManager::DELETE_VECTOR(_armyNPCs[i]);
        MemoryManager::DELETE(_faction[i]);
    }

    return Scene::deinitializeAI(continueOnErrors);
}

void WarScene::startSimulation() {
    if (g_navMeshStarted) {
        return;
    }

    g_navMeshStarted = true;
    AI::AIManager::getInstance().pauseUpdate(true);
    _infoBox->setTitle("NavMesh state");
    _infoBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_INFO);
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

        if (!navMesh->load(GET_ACTIVE_SCENEGRAPH().getRoot())) {
            loadedFromFile = false;
            AI::AIEntity::PresetAgentRadius radius =
                _army[0][0]->getAgentRadiusCategory();
            navMesh->build(
                GET_ACTIVE_SCENEGRAPH().getRoot(),
                [&radius](AI::Navigation::NavigationMesh* navMesh) {
                    AI::AIManager::getInstance().toggleNavMeshDebugDraw(true);
                    AI::AIManager::getInstance().addNavMesh(radius, navMesh);
                    g_navMeshStarted = false;
                });
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
        _lastNavMeshBuildTime = currentTime;
        for (U8 i = 0; i < 2; ++i) {
            _faction[i]->clearOrders();
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::ORDER_CAPTURE_ENEMY_FLAG));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::ORDER_SCORE_ENEMY_FLAG));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::ORDER_IDLE));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::ORDER_KILL_ENEMY));
        }
    } else {
        stringImpl info(
            "Can't reload the navigation mesh this soon.\n Please wait \\[ ");
        info.append(
            std::to_string(Time::MicrosecondsToSeconds(diffTime)).c_str());
        info.append(" ] seconds more!");

        _infoBox->setMessage(info);
        _infoBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_WARNING);
        _infoBox->show();
    }
    AI::AIManager::getInstance().pauseUpdate(false);
}

};  // namespace Divide
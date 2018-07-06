#include "Headers/WarScene.h"
#include "Headers/WarSceneAIProcessor.h"

#include "GUI/Headers/GUIMessageBox.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Core/Headers/ApplicationTimer.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"

namespace Divide {

namespace {
    static bool g_navMeshStarted = false;
};

void WarScene::printMessage(U8 eventId, const stringImpl& unitName) {
    stringImpl eventName = "";
    switch (eventId) {
        case 0: {
            eventName = "Captured flag";
        } break;
        case 1: {
            eventName = "Recovered flag";
        } break;
    };

    U32 elapsedTimeMinutes = (Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) / 60) % 60;
    U32 elapsedTimeSeconds = Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) % 60;

    Console::d_printfn(Util::StringFormat("[GAME TIME: %d:%d][%s]: %s", elapsedTimeMinutes, elapsedTimeSeconds, eventName.c_str(), unitName.c_str()).c_str());
}

void WarScene::checkGameCompletion() {
    bool restartGame = false;
    bool timeReason = false;
    if (AI::WarSceneAIProcessor::getScore(0) == _scoreLimit ||
        AI::WarSceneAIProcessor::getScore(1) == _scoreLimit) {
        _elapsedGameTime = 0;
        if (_scoreLimit == 3) {
            _scoreLimit = 5;
            _timeLimitMinutes = 8;
            restartGame = true;
        } else if (_scoreLimit == 5) {
            _scoreLimit = 20;
            _timeLimitMinutes = 30;
            restartGame = true;
        }
        else if (_scoreLimit == 20) {
            if (_runCount == 1) {
                _scoreLimit = 3;
                _timeLimitMinutes = 5;
                _runCount = 2;
                restartGame = true;
            }
            else if (_runCount == 2) {
                Application::instance().RequestShutdown();
            }
        }
    }

    U32 elapsedTimeMinutes = (Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) / 60) % 60;
    if (elapsedTimeMinutes >= _timeLimitMinutes) {
        timeReason = true;
        if (_timeLimitMinutes == 5) {
            _timeLimitMinutes = 8;
            _scoreLimit = 5;
        } else if (_timeLimitMinutes == 8) {
            _timeLimitMinutes = 30;
            _scoreLimit = 20;
        } else if (_timeLimitMinutes == 30) {
            if (_runCount == 1) {
                _timeLimitMinutes = 5;
                _scoreLimit = 3;
                _runCount = 2;
            }
            else if (_runCount == 2) {
                Application::instance().RequestShutdown();
            }
        }
        restartGame = true;
    }

    if (restartGame) {
        _elapsedGameTime = 0;
        Console::d_printfn(Util::StringFormat("\n\nRESTARTING GAME!. Reason: %s \n\n", (timeReason ? "TIME" : "SCORE")).c_str());
        AI::WarSceneAIProcessor::resetScore(0);
        AI::WarSceneAIProcessor::resetScore(1);
        if (timeReason) {
            _resetUnits = true;
            for (U8 i = 0; i < 2; ++i) {
                PhysicsComponent* flagPComp = _flag[i].lock()->get<PhysicsComponent>();
                flagPComp->popTransforms();
                _flag[i].lock()->setParent(_sceneGraph.getRoot());
                flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, i == 0 ? -206.0f : 206.0f));
            }
            AI::WarSceneAIProcessor::reset();
            AI::WarSceneAIProcessor::registerFlags(_flag[0].lock(), _flag[1].lock());
        }
    }
}

void WarScene::registerPoint(U8 teamID, const stringImpl& unitName) {
    if (!_resetUnits) {
        _resetUnits = true;

        for (U8 i = 0; i < 2; ++i) {
            PhysicsComponent* flagPComp = _flag[i].lock()->get<PhysicsComponent>();
            WAIT_FOR_CONDITION(!flagPComp->popTransforms());
            _flag[i].lock()->setParent(_sceneGraph.getRoot());
            flagPComp->setPosition(vec3<F32>(25.0f, 0.1f, i == 0 ? -206.0f : 206.0f));
        }
        AI::WarSceneAIProcessor::reset();
        AI::WarSceneAIProcessor::incrementScore(teamID);
        AI::WarSceneAIProcessor::registerFlags(_flag[0].lock(), _flag[1].lock());

        U32 elapsedTimeMinutes = (Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) / 60) % 60;
        U32 elapsedTimeSeconds = Time::MicrosecondsToSeconds<U32>(_elapsedGameTime) % 60;
        Console::d_printfn(Util::StringFormat("[GAME TIME: %d:%d][TEAM %s REGISTER POINT]: %s", elapsedTimeMinutes, elapsedTimeSeconds, (teamID == 0 ? "A" : "B"), unitName.c_str()).c_str());
        checkGameCompletion();
    }
}

bool WarScene::initializeAI(bool continueOnErrors) {
    //------------------------Artificial Intelligence------------------------//
    // Create 2 AI teams
    for (U8 i = 0; i < 2; ++i) {
        _faction[i] = MemoryManager_NEW AI::AITeam(i);
    }
    // Make the teams fight each other
    _faction[0]->addEnemyTeam(_faction[1]->getTeamID());
    _faction[1]->addEnemyTeam(_faction[0]->getTeamID());

    bool state = addUnits();
    if (state || continueOnErrors) {
        Scene::initializeAI(continueOnErrors);
    }

    _sceneGraph.findNode("Soldier1").lock()->setActive(false);
    _sceneGraph.findNode("Soldier2").lock()->setActive(false);
    _sceneGraph.findNode("Soldier3").lock()->setActive(false);

    return state;
}

bool WarScene::removeUnits(bool removeNodesOnCall) {
    WAIT_FOR_CONDITION(!AI::AIManager::instance().updating());

    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* const entity : _army[i]) {
            AI::AIManager::instance().unregisterEntity(entity);
        }

        MemoryManager::DELETE_VECTOR(_army[i]);
        MemoryManager::DELETE_VECTOR(_armyNPCs[i]);
    }

    for (U8 i = 0; i < 2; ++i) {
        for (NPC* npc : _armyNPCs[i]) {
            _sceneGraph.deleteNode(npc->getBoundNode(), removeNodesOnCall);
        }
    }

    return true;
}

bool WarScene::addUnits() {
    using namespace AI;

    // Go near the enemy flag.
    // The enemy flag will never be at the home base, because that is a scoring condition
    WarSceneAction approachEnemyFlag(ActionType::APPROACH_ENEMY_FLAG, "ApproachEnemyFlag");
    approachEnemyFlag.setPrecondition(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(false));
    approachEnemyFlag.setEffect(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(true));
    approachEnemyFlag.setEffect(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(false));

    WarSceneAction protectFlagInBase(ActionType::APPROACH_ENEMY_FLAG, "ProtectFlagInBase");
    protectFlagInBase.setPrecondition(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(false));
    protectFlagInBase.setPrecondition(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));
    protectFlagInBase.setEffect(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(true));

    // Grab the enemy flag only if we do not already have it and if we are near it
    // The enemy flag will never be at the home base, because that is a scoring condition
    WarSceneAction captureEnemyFlag(ActionType::CAPTURE_ENEMY_FLAG, "CaptureEnemyFlag");
    captureEnemyFlag.setPrecondition(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(true));
    captureEnemyFlag.setPrecondition(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));
    captureEnemyFlag.setPrecondition(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(false));
    captureEnemyFlag.setEffect(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(true));

    // Kill the enemy flag carrier to recover our own flag and return it to base
    WarSceneAction recoverFlag(ActionType::RECOVER_FLAG, "RecoverFlag");
    recoverFlag.setPrecondition(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(true));
    recoverFlag.setPrecondition(GOAPFact(Fact::ENEMY_DEAD), GOAPValue(true));
    recoverFlag.setEffect(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));

    // A general purpose "go home" action
    WarSceneAction returnToBase(ActionType::RETURN_TO_BASE, "ReturnToBase");
    returnToBase.setPrecondition(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(false));
    returnToBase.setEffect(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));

    // We can only score if we and both flags are in the same base location
    WarSceneAction scoreEnemyFlag(ActionType::SCORE_FLAG, "ScoreEnemyFlag");
    scoreEnemyFlag.setPrecondition(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));
    scoreEnemyFlag.setPrecondition(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(true));
    scoreEnemyFlag.setPrecondition(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));
    scoreEnemyFlag.setEffect(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));

    // Do nothing, but do nothing at home
    WarSceneAction idleAction(ActionType::IDLE, "Idle");
    idleAction.setPrecondition(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));
    idleAction.setEffect(GOAPFact(Fact::IDLING), GOAPFact(true));

    // Kill stuff in lack of a better occupation
    WarSceneAction attackAction(ActionType::ATTACK_ENEMY, "Attack");
    attackAction.setPrecondition(GOAPFact(Fact::ENEMY_DEAD), GOAPValue(false));
    attackAction.setEffect(GOAPFact(Fact::ENEMY_DEAD), GOAPValue(true));

    GOAPGoal recoverOwnFlag("Recover flag", to_const_uint(WarSceneOrder::WarOrder::RECOVER_FLAG));
    recoverOwnFlag.setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));
    
    GOAPGoal captureFlag("Capture enemy flag", to_const_uint(WarSceneOrder::WarOrder::CAPTURE_ENEMY_FLAG));
    captureFlag.setVariable(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(true));
    captureFlag.setVariable(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));

    GOAPGoal scoreFlag("Score", to_const_uint(WarSceneOrder::WarOrder::SCORE_ENEMY_FLAG));
    scoreFlag.setVariable(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));

    GOAPGoal idle("Idle", to_const_uint(WarSceneOrder::WarOrder::IDLE));
    idle.setVariable(GOAPFact(Fact::IDLING), GOAPValue(true));

    GOAPGoal killEnemy("Kill", to_const_uint(WarSceneOrder::WarOrder::KILL_ENEMY));
    killEnemy.setVariable(GOAPFact(Fact::ENEMY_DEAD), AI::GOAPValue(true));

    GOAPGoal protectFlagCarrier("Protect Flag Carrier", to_const_uint(WarSceneOrder::WarOrder::PROTECT_FLAG_CARRIER));
    protectFlagCarrier.setVariable(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(true));

    std::array<GOAPPackage,  to_const_uint(WarSceneAIProcessor::AIType::COUNT)> goapPackages;
    for (GOAPPackage& goapPackage : goapPackages) {
        goapPackage._worldState.setVariable(GOAPFact(Fact::AT_HOME_BASE), GOAPValue(true));
        goapPackage._worldState.setVariable(GOAPFact(Fact::ENEMY_DEAD), GOAPValue(false));
        goapPackage._worldState.setVariable(GOAPFact(Fact::ENEMY_HAS_FLAG), GOAPValue(false));
        goapPackage._worldState.setVariable(GOAPFact(Fact::HAS_ENEMY_FLAG), GOAPValue(false));
        goapPackage._worldState.setVariable(GOAPFact(Fact::IDLING), GOAPValue(true));
        goapPackage._worldState.setVariable(GOAPFact(Fact::NEAR_ENEMY_FLAG), GOAPValue(false));

        goapPackage._actionSet.push_back(idleAction);
        goapPackage._actionSet.push_back(attackAction);
        goapPackage._actionSet.push_back(recoverFlag);
        goapPackage._actionSet.push_back(protectFlagInBase);
        goapPackage._goalList.push_back(idle);
        goapPackage._goalList.push_back(killEnemy);
        goapPackage._goalList.push_back(recoverOwnFlag);
    }

    GOAPPackage& lightPackage = goapPackages[to_const_uint(WarSceneAIProcessor::AIType::LIGHT)];
    GOAPPackage& heavyPackage = goapPackages[to_const_uint(WarSceneAIProcessor::AIType::HEAVY)];

    heavyPackage._actionSet.push_back(approachEnemyFlag);
    heavyPackage._actionSet.push_back(captureEnemyFlag);
    heavyPackage._actionSet.push_back(returnToBase);
    heavyPackage._actionSet.push_back(scoreEnemyFlag);
    lightPackage._actionSet.push_back(approachEnemyFlag);

    heavyPackage._goalList.push_back(captureFlag);
    heavyPackage._goalList.push_back(scoreFlag);
    heavyPackage._goalList.push_back(protectFlagCarrier);
    lightPackage._goalList.push_back(protectFlagCarrier);

    SceneGraphNode_ptr lightNode(_sceneGraph.findNode("Soldier1").lock());
    SceneGraphNode_ptr animalNode(_sceneGraph.findNode("Soldier2").lock());
    SceneGraphNode_ptr heavyNode(_sceneGraph.findNode("Soldier3").lock());

    SceneNode* lightNodeMesh = lightNode->getNode();
    SceneNode* animalNodeMesh = animalNode->getNode();
    SceneNode* heavyNodeMesh = heavyNode->getNode();
    assert(lightNodeMesh && animalNodeMesh && heavyNodeMesh);

    NPC* soldier = nullptr;
    AIEntity* aiSoldier = nullptr;
    SceneNode* currentMesh = nullptr;

    vec3<F32> currentScale;
    stringImpl currentName;

    static const U32 normalMask = to_const_uint(SGNComponent::ComponentType::NAVIGATION) |
                                  to_const_uint(SGNComponent::ComponentType::PHYSICS) |
                                  to_const_uint(SGNComponent::ComponentType::BOUNDS) |
                                  to_const_uint(SGNComponent::ComponentType::RENDERING);

    SceneGraphNode& root = _sceneGraph.getRoot();
    for (I32 k = 0; k < 2; ++k) {
        for (I32 i = 0; i < 15; ++i) {

            F32 speed = Metric::Base(-1.0f);
            F32 acc = Metric::Base(-1.0f);

            F32 zFactor = 0.0f;
            I32 damage = 5;
            AI::WarSceneAIProcessor::AIType type;
            if (IS_IN_RANGE_INCLUSIVE(i, 0, 4)) {
                currentMesh = lightNodeMesh;
                currentScale =
                    lightNode->get<PhysicsComponent>()->getScale();
                currentName = Util::StringFormat("Soldier_1_%d_%d", k, i);
                speed = Metric::Base(Random(6.5f, 9.5f));
                acc = Metric::Base(Random(4.5f, 8.0f));
                type = AI::WarSceneAIProcessor::AIType::LIGHT;
            } else if (IS_IN_RANGE_INCLUSIVE(i, 5, 9)) {
                currentMesh = animalNodeMesh;
                currentScale =
                    animalNode->get<PhysicsComponent>()->getScale();
                currentName = Util::StringFormat("Soldier_2_%d_%d", k, i % 5);
                speed = Metric::Base(Random(8.5f, 11.5f));
                acc = Metric::Base(Random(6.0f, 9.0f));
                zFactor = 1.0f;
                type = AI::WarSceneAIProcessor::AIType::ANIMAL;
                damage = 10;
            } else {
                currentMesh = heavyNodeMesh;
                currentScale =
                    heavyNode->get<PhysicsComponent>()->getScale();
                currentName = Util::StringFormat("Soldier_3_%d_%d", k, i % 10);
                speed = Metric::Base(Random(4.5f, 7.5f));
                acc = Metric::Base(Random(4.0f, 6.5f));
                zFactor = 2.0f;
                type = AI::WarSceneAIProcessor::AIType::HEAVY;
                damage = 15;
            }

            SceneGraphNode_ptr currentNode = root.addNode(*currentMesh, normalMask, currentName);
            currentNode->setSelectable(true);

            PhysicsComponent* pComp =
                currentNode->get<PhysicsComponent>();
            pComp->setScale(currentScale);

            if (k == 0) {
                zFactor *= -25;
                zFactor -= 200;
                pComp->translateX(Metric::Base(-25.0f));
            } else {
                zFactor *= 25;
                zFactor += 200;
                pComp->rotateY(Angle::Degrees(180.0f));
                pComp->translateX(Metric::Base(100.0f));
                pComp->translateX(Metric::Base(25.0f));
            }

            pComp->setPosition(vec3<F32>(Metric::Base(-125.0f + 25 * (i % 5)),
                                         Metric::Base(-0.01f),
                                         Metric::Base(zFactor)));

            aiSoldier = MemoryManager_NEW AI::AIEntity(pComp->getPosition(),
                                                       currentNode->getName());
            aiSoldier->addSensor(AI::SensorType::VISUAL_SENSOR);
            k == 0
                ? currentNode->get<RenderingComponent>()
                      ->renderBoundingBox(true)
                : currentNode->get<RenderingComponent>()
                      ->renderSkeleton(true);

            AI::WarSceneAIProcessor* brain =
                MemoryManager_NEW AI::WarSceneAIProcessor(type);

            // GOAP
            brain->registerGOAPPackage(goapPackages[to_uint(type)]);
            aiSoldier->setAIProcessor(brain);
            soldier = MemoryManager_NEW NPC(currentNode, aiSoldier);
            soldier->setAttribute(to_const_uint(AI::UnitAttributes::HEALTH_POINTS), 100);
            soldier->setAttribute(to_const_uint(AI::UnitAttributes::DAMAGE), damage);
            soldier->setAttribute(to_const_uint(AI::UnitAttributes::ALIVE_FLAG), 1);
            soldier->setMovementSpeed(speed);
            soldier->setAcceleration(acc);

            _army[k].push_back(aiSoldier);
            _armyNPCs[k].push_back(soldier);
            
        }
    }

    //----------------------- AI controlled units ---------------------//
    for (U8 i = 0; i < 2; ++i) {
        for (U8 j = 0; j < _army[i].size(); ++j) {
            AI::AIManager::instance().registerEntity(i, _army[i][j]);
        }
    }

    return !(_army[0].empty() || _army[1].empty());
}

AI::AIEntity* WarScene::findAI(SceneGraphNode_ptr node) {
    I64 targetGUID = node->getGUID();

    for (U8 i = 0; i < 2; ++i) {
        for (AI::AIEntity* entity : _army[i]) {
            if (entity->getUnitRef()->getBoundNode().lock()->getGUID() == targetGUID) {
                return entity;
            }
        }
    }

    return nullptr;
}

bool WarScene::resetUnits() {
    AI::AIManager::instance().pauseUpdate(true);
    bool state = false;
    if (removeUnits(true)) {
       state = addUnits();
    }
    AI::AIManager::instance().pauseUpdate(false);
    return state;
}

bool WarScene::deinitializeAI(bool continueOnErrors) {
    AI::AIManager::instance().pauseUpdate(true);
    if (removeUnits(true)) {
        for (U8 i = 0; i < 2; ++i) {
            MemoryManager::DELETE(_faction[i]);
        }
        return Scene::deinitializeAI(continueOnErrors);
    }

    return false;
}

void WarScene::startSimulation() {
    if (g_navMeshStarted) {
        return;
    }

    g_navMeshStarted = true;

    AI::AIManager::instance().pauseUpdate(true);
    _infoBox->setTitle("NavMesh state");
    _infoBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_INFO);
    bool previousMesh = false;
    bool loadedFromFile = true;
    U64 currentTime = Time::ElapsedMicroseconds(true);
    U64 diffTime = currentTime - _lastNavMeshBuildTime;
    if (_lastNavMeshBuildTime == 0UL ||
        diffTime > Time::SecondsToMicroseconds(10)) {
        AI::Navigation::NavigationMesh* navMesh =
            AI::AIManager::instance().getNavMesh(
                _army[0][0]->getAgentRadiusCategory());
        if (navMesh) {
            previousMesh = true;
            AI::AIManager::instance().destroyNavMesh(
                _army[0][0]->getAgentRadiusCategory());
        }
        navMesh = MemoryManager_NEW AI::Navigation::NavigationMesh();
        navMesh->setFileName(getName());

        if (!navMesh->load(_sceneGraph.getRoot())) {
            loadedFromFile = false;
            AI::AIEntity::PresetAgentRadius radius =
                _army[0][0]->getAgentRadiusCategory();
            navMesh->build(
                _sceneGraph.getRoot(),
                [&radius](AI::Navigation::NavigationMesh* navMesh) {
                AI::AIManager::instance().toggleNavMeshDebugDraw(true);
                AI::AIManager::instance().addNavMesh(radius, navMesh);
                g_navMeshStarted = false;
            });
        }
        else {
            AI::AIManager::instance().addNavMesh(
                _army[0][0]->getAgentRadiusCategory(), navMesh);
#ifdef _DEBUG
            navMesh->debugDraw(true);
#endif
            renderState().drawDebugTargetLines(true);
        }

        if (previousMesh) {
            if (loadedFromFile) {
                _infoBox->setMessage(
                    "Re-loaded the navigation mesh from file!");
            }
            else {
                _infoBox->setMessage(
                    "Re-building the navigation mesh in a background thread!");
            }
        }
        else {
            if (loadedFromFile) {
                _infoBox->setMessage("Navigation mesh loaded from file!");
            }
            else {
                _infoBox->setMessage(
                    "Navigation mesh building in a background thread!");
            }
        }
        //_infoBox->show();
        _lastNavMeshBuildTime = currentTime;
        for (U8 i = 0; i < 2; ++i) {
            _faction[i]->clearOrders();
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::CAPTURE_ENEMY_FLAG));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::SCORE_ENEMY_FLAG));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::IDLE));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::KILL_ENEMY));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::PROTECT_FLAG_CARRIER));
            _faction[i]->addOrder(std::make_shared<AI::WarSceneOrder>(
                AI::WarSceneOrder::WarOrder::RECOVER_FLAG));
        }
    } else {
        stringImpl info(
            "Can't reload the navigation mesh this soon.\n Please wait \\[ ");
        info.append(
            to_stringImpl(Time::MicrosecondsToSeconds<I32>(diffTime)).c_str());
        info.append(" ] seconds more!");

        _infoBox->setMessage(info);
        _infoBox->setMessageType(GUIMessageBox::MessageType::MESSAGE_WARNING);
        _infoBox->show();
    }

    AI::AIManager::instance().pauseUpdate(false);
}

};  // namespace Divide

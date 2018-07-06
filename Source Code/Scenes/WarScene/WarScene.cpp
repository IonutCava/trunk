#include "Headers/WarScene.h"
#include "Headers/WarSceneAISceneImpl.h"

#include "Geometry/Material/Headers/Material.h"
#include "Core/Math/Headers/Transform.h"
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

REGISTER_SCENE(WarScene);

void WarScene::processGUI(const U64 deltaTime){
    D32 FpsDisplay = getSecToMs(0.3);
    if (_guiTimers[0] >= FpsDisplay){
        const Camera& cam = renderState().getCamera();
        const vec3<F32>& eyePos = cam.getEye();
        const vec3<F32>& euler = cam.getEuler();
        //const vec3<F32>& lampPos = _lampLightNode->getComponent<PhysicsComponent>()->getConstTransform()->getPosition();
        _GUI->modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        _GUI->modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
        _GUI->modifyText("camPosition", "Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
            eyePos.x, eyePos.y, eyePos.z, euler.pitch, euler.yaw);
        /*_GUI->modifyText("lampPosition","Lamp Position  [ X: %5.2f | Y: %5.2f | Z: %5.2f ]",
        lampPos.x, lampPos.y, lampPos.z);*/

        _guiTimers[0] = 0.0;
    }
    Scene::processGUI(deltaTime);
}

void WarScene::processTasks(const U64 deltaTime){
    if(!_sceneReady) return;

    static vec2<F32> _sunAngle = vec2<F32>(0.0f, RADIANS(45.0f));
    static bool direction = false;
    if(!direction){
        _sunAngle.y += 0.005f;
        _sunAngle.x += 0.005f;
    }else{
        _sunAngle.y -= 0.005f;
        _sunAngle.x -= 0.005f;
    }

    if(_sunAngle.y  <= RADIANS(25) || _sunAngle.y >= RADIANS(70))
        direction = !direction;

    _sunvector = vec3<F32>(	-cosf(_sunAngle.x) * sinf(_sunAngle.y),
                            -cosf(_sunAngle.y),
                            -sinf(_sunAngle.x) * sinf(_sunAngle.y));

    _sun->setDirection(_sunvector);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);

    D32 BobTimer = getSecToMs(5);
    D32 DwarfTimer = getSecToMs(8);
    D32 BullTimer = getSecToMs(10);

    if(_taskTimers[0] >= BobTimer){
        if(_bobNode)
            _bobNode->getComponent<AnimationComponent>()->playNextAnimation();

        _taskTimers[0] = 0.0;
    }
    if(_taskTimers[1] >= DwarfTimer){
        SceneGraphNode* dwarf = _sceneGraph->findNode("Soldier1");
         if(dwarf)
             dwarf->getComponent<AnimationComponent>()->playNextAnimation();
        _taskTimers[1] = 0.0;
    }
    if(_taskTimers[2] >= BullTimer){
        SceneGraphNode* bull = _sceneGraph->findNode("Soldier2");
         if(bull)
             bull->getComponent<AnimationComponent>()->playNextAnimation();
        _taskTimers[2] = 0.0;
    }
    Scene::processTasks(deltaTime);
}

static boost::atomic_bool navMeshStarted;

void navMeshCreationCompleteCallback(AIEntity::PresetAgentRadius radius,  Navigation::NavigationMesh* navMesh){
    navMesh->save();
    AIManager::getInstance().addNavMesh(radius, navMesh);
    //AIManager::getInstance().toggleNavMeshDebugDraw(navMesh, true);
}

void WarScene::resetSimulation(){
    navMeshStarted = false;
}

void WarScene::startSimulation(){
    resetSimulation();

    if (!navMeshStarted) {
        Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(_army1[0]->getAgentRadiusCategory());
        if (!navMesh) {
            navMesh = New Navigation::NavigationMesh();
        }
        navMesh->setFileName(GET_ACTIVE_SCENE()->getName());

        if (!navMesh->load(nullptr)) {
            navMesh->build(nullptr, DELEGATE_BIND(navMeshCreationCompleteCallback, _army1[0]->getAgentRadiusCategory(), navMesh));
        } else {
            AIManager::getInstance().addNavMesh(_army1[0]->getAgentRadiusCategory(), navMesh);
#       ifdef _DEBUG
                AIManager::getInstance().toggleNavMeshDebugDraw(navMesh, true);
#       endif
        }

        navMeshStarted = true;
    }

}

void WarScene::processSimulation(cdiggins::any a, CallbackParam b){
    if(getTasks().empty()) return;
    //SceneGraphNode* Soldier1 = _sceneGraph->findNode("Soldier1");
    //assert(Soldier1);
    //assert(_groundPlaceholder);


}

void WarScene::processInput(const U64 deltaTime){
}

void WarScene::updateSceneStateInternal(const U64 deltaTime){
    if (!_sceneReady) {
        return;
    }
    static U64 totalTime = 0;
    
    totalTime += deltaTime;

    /*if(_lampLightNode && _bobNodeBody){
        static mat4<F32> position = _lampLightNode->getComponent<PhysicsComponent>()->getConstTransform()->getMatrix(); 
        const mat4<F32>& fingerPosition = _bobNodeBody->getAnimationComponent()->getBoneTransform("fingerstip.R");
        mat4<F32> finalTransform(fingerPosition * position);
        _lampLightNode->getComponent<PhysicsComponent>()->setTransforms(finalTransform.transpose());
    }*/
    
#ifdef _DEBUG
    if (!AIManager::getInstance().getNavMesh(_army1[0]->getAgentRadiusCategory())) {
        return;
    }

    U32 characterCount = (U32)(_army1.size() + _army2.size());

    _lines[DEBUG_LINE_OBJECT_TO_TARGET].resize(characterCount);
    //renderState().drawDebugLines(true);
    U32 count = 0;
    for(AIEntity* character : _army1){
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._startPoint.set(character->getPosition());
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._endPoint.set(character->getDestination());
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._color.set(255,0,255,128);
        count++;
    }
    
    for(AIEntity* character : _army2){
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._startPoint.set(character->getPosition());
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._endPoint.set(character->getDestination());
        _lines[DEBUG_LINE_OBJECT_TO_TARGET][count]._color.set(255,0,255,128);
        count++;
    }
#endif
}

bool WarScene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const gui){
    navMeshStarted = false;
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,gui,true,true);
    //Add a light
    _sun = addDefaultLight();
    //Add a skybox
    addDefaultSky();
    //Position camera
    renderState().getCamera().setEye(vec3<F32>(54.5f, 25.5f, 1.5f));
    renderState().getCamera().setGlobalRotation(-90/*yaw*/,35/*pitch*/);
    _sun->csmSplitCount(3); // 3 splits
    _sun->csmSplitLogFactor(0.925f);
    _sun->csmNearClipOffset(25.0f);
    // Add some obstacles
    SceneGraphNode* cylinderNW = _sceneGraph->findNode("cylinderNW");
    SceneGraphNode* cylinderNE = _sceneGraph->findNode("cylinderNE");
    SceneGraphNode* cylinderSW = _sceneGraph->findNode("cylinderSW");
    SceneGraphNode* cylinderSE = _sceneGraph->findNode("cylinderSE");

    SceneGraphNode::NodeChildren& children = cylinderNW->getChildren();
    (*children.begin()).second->getNode()->getMaterial()->setDoubleSided(true);
    

    assert(cylinderNW && cylinderNE && cylinderSW && cylinderSE);
    SceneNode* cylinderMeshNW = cylinderNW->getNode();
    SceneNode* cylinderMeshNE = cylinderNE->getNode();
    SceneNode* cylinderMeshSW = cylinderSW->getNode();
    SceneNode* cylinderMeshSE = cylinderSE->getNode();

    std::string currentName;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode* baseNode = nullptr;
    SceneGraphNode* currentNode = nullptr;
    std::pair<I32, I32> currentPos;
    for(U8 i = 0; i < 40; ++i){
        if(i < 10){
            baseNode = cylinderNW;
            currentMesh = cylinderMeshNW;
            currentName = std::string("Cylinder_NW_" + Util::toString((I32)i));
            currentPos.first  = -200 + 40 * i + 50;
            currentPos.second = -200 + 40 * i + 50;
        }else if(i >= 10 && i < 20){
            baseNode = cylinderNE;
            currentMesh = cylinderMeshNE;
            currentName = std::string("Cylinder_NE_" + Util::toString((I32)i));
            currentPos.first  =  200 - 40 * (i%10) - 50;
            currentPos.second = -200 + 40 * (i%10) + 50;
        }else if(i >= 20 && i < 30){
            baseNode = cylinderSW;
            currentMesh = cylinderMeshSW;
            currentName = std::string("Cylinder_SW_" + Util::toString((I32)i));
            currentPos.first  = -200 + 40 * (i%20) + 50;
            currentPos.second =  200 - 40 * (i%20) - 50;
        }else{
            baseNode = cylinderSE;
            currentMesh = cylinderMeshSE;
            currentName = std::string("Cylinder_SE_" + Util::toString((I32)i));
            currentPos.first  = 200 - 40 * (i%30) - 50;
            currentPos.second = 200 - 40 * (i%30) - 50;
        }

        currentNode = _sceneGraph->getRoot()->addNode(currentMesh, currentName);
        assert(currentNode);
        currentNode->setSelectable(true);
        currentNode->usageContext(baseNode->usageContext());
        currentNode->getComponent<PhysicsComponent>()->physicsGroup(baseNode->getComponent<PhysicsComponent>()->physicsGroup());
        currentNode->getComponent<NavigationComponent>()->navigationContext(baseNode->getComponent<NavigationComponent>()->navigationContext());
        currentNode->getComponent<NavigationComponent>()->navigationDetailOverride(baseNode->getComponent<NavigationComponent>()->navMeshDetailOverride());
        
        currentNode->getComponent<PhysicsComponent>()->setScale(baseNode->getComponent<PhysicsComponent>()->getConstTransform()->getScale());
        currentNode->getComponent<PhysicsComponent>()->setPosition(vec3<F32>(currentPos.first, -0.01f, currentPos.second));
    }

    /*_bobNode = _sceneGraph->findNode("Soldier3");
    _bobNodeBody = _sceneGraph->findNode("Soldier3_Bob.md5mesh-submesh-0");
    _lampLightNode = nullptr;
    if(_bobNodeBody != nullptr){
        ResourceDescriptor tempLight("Light_lamp");
        tempLight.setId(2);
        tempLight.setEnumValue(LIGHT_TYPE_POINT);
        // Create a point light for Bob's lamp
        /*Light* light = CreateResource<Light>(tempLight);
        light->setDrawImpostor(true);
        // Make it small and yellow
        light->setCastShadows(false);
        light->setRange(2.0f);
        light->setDiffuseColor(vec3<F32>(1.0f, 0.5f, 0.0f));
        _lampTransform = new SceneTransform();
        // Add it to Bob's body
        _lampTransformNode = _bobNodeBody->addNode(_lampTransform, "lampTransform");
        _lampLightNode = addLight(light, _lampTransformNode);
        // Move it to the lamp's position
        _lampTransformNode->getComponent<PhysicsComponent>()->setPosition(vec3<F32>(-75.0f, -45.0f, -5.0f));
    }*/
    //------------------------ The rest of the scene elements -----------------------------///


    ParticleEmitterDescriptor particleSystem;
#ifdef _DEBUG
    particleSystem._particleCount = 200;
#else
    particleSystem._particleCount = 20000;
#endif
    particleSystem._spread = 5.0f;

    ParticleEmitter* test = addParticleEmitter("TESTPARTICLES", particleSystem);
    SceneGraphNode* testSGN = _sceneGraph->getRoot()->addNode(test, "TESTPARTICLES");
    testSGN->getComponent<PhysicsComponent>()->translateY(5);
    test->setDrawImpostor(true);
    test->enableEmitter(true);
    _sceneReady = true;
    return loadState;
}

bool WarScene::initializeAI(bool continueOnErrors){
    //----------------------------Artificial Intelligence------------------------------//
    
    // Create 2 AI teams
    _faction1 = New AITeam(1);
    _faction2 = New AITeam(2);
    // Make the teams fight eachother
    _faction1->addEnemyTeam(_faction2->getTeamID());
    _faction2->addEnemyTeam(_faction1->getTeamID());


    SceneGraphNode* soldierNode1 = _sceneGraph->findNode("Soldier1");
    SceneGraphNode* soldierNode2 = _sceneGraph->findNode("Soldier2");
    SceneGraphNode* soldierNode3 = _sceneGraph->findNode("Soldier3");
    SceneNode* soldierMesh1 = soldierNode1->getNode();
    SceneNode* soldierMesh2 = soldierNode2->getNode();
    SceneNode* soldierMesh3 = soldierNode3->getNode();
    assert(soldierMesh1 && soldierMesh2 && soldierMesh3);

    vec3<F32> currentScale;
    NPC* soldier = nullptr;
    std::string currentName;
    AIEntity* aiSoldier = nullptr;
    SceneNode* currentMesh = nullptr;
    SceneGraphNode* currentNode = nullptr;
    for(U8 k = 0; k < 2; ++k) {
        for(U8 i = 0; i < 15; ++i){
            F32 speed = 5.5f; // 5.5 m/s
            U8 zFactor = 0;
            if(i < 5){
                currentMesh = soldierMesh1;
                currentScale = soldierNode1->getComponent<PhysicsComponent>()->getConstTransform()->getScale();
                currentName = std::string("Soldier_1_" + Util::toString((I32)k) + "_" + Util::toString((I32)i));
            }else if(i >= 5 && i < 10){
                currentMesh = soldierMesh2;
                currentScale = soldierNode2->getComponent<PhysicsComponent>()->getConstTransform()->getScale();
                currentName = std::string("Soldier_2_" + Util::toString((I32)k) + "_" + Util::toString((I32)i%5));
                speed = 5.75f;
                zFactor = 1;
            }else{
                currentMesh = soldierMesh3;
                currentScale = soldierNode3->getComponent<PhysicsComponent>()->getConstTransform()->getScale();
                currentName = std::string("Soldier_3_" + Util::toString((I32)k) + "_" + Util::toString((I32)i%10));
                speed = 5.35f;
                zFactor = 2;
            }

            currentNode = _sceneGraph->getRoot()->addNode(currentMesh, currentName);
            currentNode->getComponent<PhysicsComponent>()->setScale(currentScale);
            DIVIDE_ASSERT(currentNode != nullptr, "WarScene error: INVALID SOLDIER NODE TEMPLATE!");
            currentNode->setSelectable(true);
            I8 side = k == 0 ? -1 : 1;

            currentNode->getComponent<PhysicsComponent>()->setPosition(vec3<F32>(-125 + 25*(i%5), -0.01f, 200 * side + 25*zFactor*side));
            if (side == 1) {
                 currentNode->getComponent<PhysicsComponent>()->rotateY(180);
                 currentNode->getComponent<PhysicsComponent>()->translateX(100);
            }
          
            currentNode->getComponent<PhysicsComponent>()->translateX(25 * side);

            aiSoldier = New AIEntity(currentNode->getComponent<PhysicsComponent>()->getConstTransform()->getPosition(), currentNode->getName());
            aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
            aiSoldier->addAISceneImpl(New WarSceneAISceneImpl(_GOAPContext));
            aiSoldier->setTeam(k == 0 ? _faction1 : _faction2);
            soldier = New NPC(currentNode, aiSoldier);
            soldier->setMovementSpeed(speed); 
            if(k == 0){
                _army1NPCs.push_back(soldier);
                _army1.push_back(aiSoldier);
            }else{
                _army2NPCs.push_back(soldier);
                _army2.push_back(aiSoldier);
            }
        }
    }

    //----------------------- AI controlled units ---------------------//
    for(U8 i = 0; i < _army1.size(); i++){
        AIManager::getInstance().addEntity(_army1[i]);    
    }

    for(U8 i = 0; i < _army2.size(); i++){
        AIManager::getInstance().addEntity(_army2[i]);
    }

    bool state = !(_army1.empty() || _army2.empty());

    if(state || continueOnErrors) Scene::initializeAI(continueOnErrors);

    _sceneGraph->getRoot()->removeNode(soldierNode1);
    _sceneGraph->getRoot()->removeNode(soldierNode2);
    _sceneGraph->getRoot()->removeNode(soldierNode3);
    SAFE_DELETE(soldierNode1);
    SAFE_DELETE(soldierNode2);
    SAFE_DELETE(soldierNode3);
    AIManager::getInstance().pauseUpdate(false);
    return state;
}

bool WarScene::deinitializeAI(bool continueOnErrors){
    AIManager::getInstance().pauseUpdate(true);
    for(U8 i = 0; i < _army1NPCs.size(); i++){
        SAFE_DELETE(_army1NPCs[i]);
    }
    _army1NPCs.clear();
    for(U8 i = 0; i < _army2NPCs.size(); i++){
        SAFE_DELETE(_army2NPCs[i]);
    }
    _army2NPCs.clear();
    for(U8 i = 0; i < _army1.size(); i++){
        AIManager::getInstance().destroyEntity(_army1[i]->getGUID());
    }
    _army1.clear();
    for(U8 i = 0; i < _army2.size(); i++){
        AIManager::getInstance().destroyEntity(_army2[i]->getGUID());
    }
    _army2.clear();
    SAFE_DELETE(_faction1);
    SAFE_DELETE(_faction2);
    return Scene::deinitializeAI(continueOnErrors);
}

bool WarScene::unload(){
    return Scene::unload();
}

bool WarScene::loadResources(bool continueOnErrors){
    _GUI->addButton("Simulate", "Simulate", vec2<I32>(renderState().cachedResolution().width-220 ,
                                                      renderState().cachedResolution().height/1.1f),
                                                      vec2<U32>(100,25),vec3<F32>(0.65f),
                                                      DELEGATE_BIND(&WarScene::startSimulation,this));

    _GUI->addText("fpsDisplay",           //Unique ID
                  vec2<I32>(60,60),          //Position
                  Font::DIVIDE_DEFAULT,    //Font
                  vec3<F32>(0.0f,0.2f, 1.0f),  //Color
                  "FPS: %s",0);    //Text and arguments
    _GUI->addText("RenderBinCount",
                  vec2<I32>(60,70),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.6f,0.2f,0.2f),
                  "Number of items in Render Bin: %d",0);

    _GUI->addText("camPosition",  vec2<I32>(60,100),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f,0.8f,0.2f),
                  "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
                  renderState().getCamera().getEye().x,
                  renderState().getCamera().getEye().y,
                  renderState().getCamera().getEye().z,
                  renderState().getCamera().getEuler().pitch,
                  renderState().getCamera().getEuler().yaw);

    _GUI->addText("lampPosition",  vec2<I32>(60,120),
                  Font::DIVIDE_DEFAULT,
                  vec3<F32>(0.2f,0.8f,0.2f),
                  "Lamp Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",
                  0.0f, 0.0f, 0.0f);
    //Add a first person camera
    Camera* cam = New FirstPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(10.0f);
    cam->setTurnSpeedFactor(10.0f);
    renderState().getCameraMgr().addNewCamera("fpsCamera", cam);
    //Add a third person camera
    cam = New ThirdPersonCamera();
    cam->fromCamera(renderState().getCameraConst());
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);
    renderState().getCameraMgr().addNewCamera("tpsCamera", cam);

    _guiTimers.push_back(0.0); //Fps
    _taskTimers.push_back(0.0); //animation bull
    _taskTimers.push_back(0.0); //animation dwarf
    _taskTimers.push_back(0.0); //animation bob
    return true;
}

bool WarScene::onKeyUp(const OIS::KeyEvent& key){
    static bool fpsCameraActive = false;
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    bool keyState = Scene::onKeyUp(key);
    switch(key.key)	{
        default: break;

        case OIS::KC_TAB:{
            if(_currentSelection != nullptr){
                /*if(flyCameraActive){
                    renderState().getCameraMgr().pushActiveCamera("fpsCamera");
                    flyCameraActive = false; fpsCameraActive = true;
                }else if(fpsCameraActive){*/
                if(flyCameraActive){
                    if(fpsCameraActive) renderState().getCameraMgr().popActiveCamera();
                    renderState().getCameraMgr().pushActiveCamera("tpsCamera");
                    static_cast<ThirdPersonCamera&>(renderState().getCamera()).setTarget(_currentSelection);
                    /*fpsCameraActive*/flyCameraActive = false; tpsCameraActive = true;
                    break;
                }
            }
            if (tpsCameraActive){
                renderState().getCameraMgr().pushActiveCamera("defaultCamera");
                tpsCameraActive = false; flyCameraActive = true;
            }
//			renderState().getCamera().setTargetNode(_currentSelection);
        }break;
    }
    return keyState;
}

bool WarScene::mouseMoved(const OIS::MouseEvent& key){
    if(_mousePressed[OIS::MB_Right]){
        if(_previousMousePos.x - key.state.X.abs > 1 )   	 state()._angleLR = -1;
        else if(_previousMousePos.x - key.state.X.abs < -1 ) state()._angleLR =  1;
        else 			                                     state()._angleLR =  0;

        if(_previousMousePos.y - key.state.Y.abs > 1 )		 state()._angleUD = -1;
        else if(_previousMousePos.y - key.state.Y.abs < -1 ) state()._angleUD =  1;
        else 	                                             state()._angleUD =  0;
    }

    return Scene::mouseMoved(key);
}

bool WarScene::mouseButtonPressed(const OIS::MouseEvent& key, OIS::MouseButtonID button){
    return Scene::mouseButtonPressed(key,button);
}

bool WarScene::mouseButtonReleased(const OIS::MouseEvent& key, OIS::MouseButtonID button){
    bool keyState = Scene::mouseButtonReleased(key,button);
    if(!_mousePressed[OIS::MB_Right]){
        state()._angleUD = 0;
        state()._angleLR = 0;
    }
    return keyState;
}
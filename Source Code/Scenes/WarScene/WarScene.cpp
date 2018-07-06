#include "Headers/WarScene.h"
#include "Headers/WarSceneAIActionList.h"

#include "Core/Math/Headers/Transform.h"
#include "Geometry/Shapes/Headers/SkinnedMesh.h"
#include "Geometry/Shapes/Headers/SkinnedSubMesh.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Rendering/Camera/Headers/ThirdPersonCamera.h"
#include "Rendering/Camera/Headers/FirstPersonCamera.h"
#include "Rendering/RenderPass/Headers/RenderQueue.h"
#include "Dynamics/Entities/Units/Headers/NPC.h"
#include "Managers/Headers/CameraManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/AIManager.h"
#include "Rendering/Headers/Frustum.h"
#include "AI/PathFinding/Headers/DivideRecast.h"

REGISTER_SCENE(WarScene);

void WarScene::preRender(){
    
}

void WarScene::processTasks(const U64 deltaTime){

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

    LightManager::getInstance().getLight(0)->setDirection(_sunvector);
    getSkySGN(0)->getNode<Sky>()->setSunVector(_sunvector);

    D32 FpsDisplay = getSecToMs(0.3);
    D32 BobTimer = getSecToMs(5);
    D32 DwarfTimer = getSecToMs(8);
    D32 BullTimer = getSecToMs(10);

    if (_taskTimers[0] >= FpsDisplay){
        const Camera& cam = renderState().getCamera();
        //const vec3<F32>& lampPos = _lampLightNode->getTransform()->getPosition();
        GUI::getInstance().modifyText("fpsDisplay", "FPS: %3.0f. FrameTime: %3.1f", ApplicationTimer::getInstance().getFps(), ApplicationTimer::getInstance().getFrameTime());
        GUI::getInstance().modifyText("RenderBinCount", "Number of items in Render Bin: %d", GFX_RENDER_BIN_SIZE);
        GUI::getInstance().modifyText("camPosition","Position [ X: %5.2f | Y: %5.2f | Z: %5.2f ] [Pitch: %5.2f | Yaw: %5.2f]",
                              cam.getEye().x,
                              cam.getEye().y,
                              cam.getEye().z,
                              cam.getEuler().pitch,
                              cam.getEuler().yaw);
        /*GUI::getInstance().modifyText("lampPosition","Lamp Position  [ X: %5.2f | Y: %5.2f | Z: %5.2f ]",
                                      lampPos.x,
                                      lampPos.y,
                                      lampPos.z);*/
        
        _taskTimers[0] = 0.0;
    }
    if(_taskTimers[1] >= BobTimer){
        if(_bobNode)
            _bobNode->getNode<SkinnedMesh>()->playNextAnimation();

        _taskTimers[1] = 0.0;
    }
    if(_taskTimers[2] >= DwarfTimer){
        SceneGraphNode* dwarf = _sceneGraph->findNode("Soldier1");
         if(dwarf)
            dwarf->getNode<SkinnedMesh>()->playNextAnimation();
        _taskTimers[2] = 0.0;
    }
    if(_taskTimers[3] >= BullTimer){
        SceneGraphNode* bull = _sceneGraph->findNode("Soldier2");
         if(bull)
            bull->getNode<SkinnedMesh>()->playNextAnimation();
        _taskTimers[3] = 0.0;
    }
    Scene::processTasks(deltaTime);
}

void WarScene::resetSimulation(){
    clearTasks();
}

void WarScene::startSimulation(){
    resetSimulation();

    if(getTasks().empty()){
        Kernel* kernel = Application::getInstance().getKernel();
        Task_ptr newSim(New Task(kernel->getThreadPool(),12,true,false,DELEGATE_BIND(&WarScene::processSimulation,this,rand() % 5,TYPE_INTEGER)));
        addTask(newSim);
    }

}

void WarScene::processSimulation(boost::any a, CallbackParam b){
    if(getTasks().empty()) return;
    //SceneGraphNode* Soldier1 = _sceneGraph->findNode("Soldier1");
    //assert(Soldier1);
    //assert(_groundPlaceholder);
}

void WarScene::processInput(const U64 deltaTime){
    if(state()._moveFB)  renderState().getCamera().moveForward(state()._moveFB);
    if(state()._moveLR)  renderState().getCamera().moveStrafe(state()._moveLR);

    Camera& cam = renderState().getCamera();
    switch(cam.getType()){
        default:
        case Camera::FREE_FLY:{
            if(state()._angleLR) renderState().getCamera().rotateYaw(state()._angleLR);
            if(state()._angleUD) renderState().getCamera().rotatePitch(state()._angleUD);
            if(state()._roll)    renderState().getCamera().rotateRoll(state()._roll);
        }break;
    }
}

void WarScene::updateSceneState(const U64 deltaTime){
    static U64 totalTime = 0;
    static bool navMeshCreated = false;
    totalTime += deltaTime;
    if(getUsToSec(totalTime) > 20 && !navMeshCreated && false){
        Navigation::NavigationMesh* temp = New Navigation::NavigationMesh();
        temp->setFileName(GET_ACTIVE_SCENE()->getName());
        bool loaded = temp->load(NULL);//<Start from root for now

        if(!loaded){
            loaded = temp->build(NULL,false);
            temp->save();
        }

        if(loaded){
            navMeshCreated = AIManager::getInstance().addNavMesh(temp);
        }
    }
    Scene::updateSceneState(deltaTime);
    if(_lampLightNode && _bobNodeBody){
      /*  static mat4<F32> position = _lampLightNode->getTransform()->getMatrix(); 
        const mat4<F32>& fingerPosition = _bobNodeBody->getBoneTransform("fingerstip.R");
        mat4<F32> finalTransform(fingerPosition * position);
        _lampLightNode->getTransform()->setTransforms(finalTransform.transpose());*/
    }
        
    Navigation::NavigationMesh* navMesh = AIManager::getInstance().getNavMesh(0);

    for_each(AIEntity* character, _army1){

         // Update character (position, animations, state)
        character->update(deltaTime);
        // If destination reached: Set new random destination
        if ( character->destinationReached() ) {
            character->updateDestination(navMesh ? Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(*navMesh) : _army2[0]->getUnitRef()->getPosition());
        }
    }

    
    for_each(AIEntity* character, _army2){

         // Update character (position, animations, state)
        character->update(deltaTime);
        // If destination reached: Set new random destination
        if ( character->destinationReached() ) {
            character->updateDestination( navMesh ? Navigation::DivideRecast::getInstance().getRandomNavMeshPoint(*navMesh) : _army1[0]->getUnitRef()->getPosition());
        }
    }
}

bool WarScene::load(const std::string& name, CameraManager* const cameraMgr){
    //Load scene resources
    bool loadState = SCENE_LOAD(name,cameraMgr,true,true);
    //Add a light
    addDefaultLight();
    //Add a skybox
    addDefaultSky();
    //Position camera
    renderState().getCamera().setEye(vec3<F32>(14,5.5f,11.5f));
    renderState().getCamera().setGlobalRotation(-45/*yaw*/,25/*pitch*/);
    //Create 2 AI teams
    _faction1 = New AICoordination(1);
    _faction2 = New AICoordination(2);

    _bobNode = _sceneGraph->findNode("Soldier3");
    _bobNodeBody = _sceneGraph->findNode("Soldier3_Bob.md5mesh-submesh-0");
    _lampLightNode = NULL;
    if(_bobNodeBody != NULL){
        ResourceDescriptor tempLight("Light_lamp");
        tempLight.setId(2);
        tempLight.setEnumValue(LIGHT_TYPE_POINT);
        // Create a point light for Bob's lamp
        /*Light* light = CreateResource<Light>(tempLight);
        light->setDrawImpostor(true);
        // Make it small and yellow
        light->setCastShadows(false);
        light->setLightProperties(LIGHT_PROPERTY_BRIGHTNESS, 2.0f);
        light->setLightProperties(LIGHT_PROPERTY_DIFFUSE, vec3<F32>(1.0f, 0.5f, 0.0f));
        _lampTransform = new SceneTransform();
        // Add it to Bob's body
        _lampTransformNode = _bobNodeBody->addNode(_lampTransform, "lampTransform");
        _lampLightNode = addLight(light, _lampTransformNode);
        // Move it to the lamp's position
        _lampTransformNode->getTransform()->setPosition(vec3<F32>(-75.0f, -45.0f, -5.0f));*/
    }
    //------------------------ The rest of the scene elements -----------------------------///
//	_groundPlaceholder = _sceneGraph->findNode("Ground_placeholder");
//	_groundPlaceholder->getNode<SceneNode>()->getMaterial()->setCastsShadows(false);

    return loadState;
}

bool WarScene::initializeAI(bool continueOnErrors){
    //----------------------------Artificial Intelligence------------------------------//

    AIEntity* aiSoldier = NULL;
    SceneGraphNode* soldierMesh = _sceneGraph->findNode("Soldier1");
    if(soldierMesh){
        soldierMesh->setSelectable(true);
        AIEntity* aiSoldier = New AIEntity(soldierMesh->getTransform()->getPosition(), "Soldier1");
        aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
        aiSoldier->setComInterface();
        aiSoldier->addActionProcessor(New WarSceneAIActionList());
        aiSoldier->setTeam(_faction1);
        NPC* soldier = New NPC(soldierMesh, aiSoldier);
        soldier->setMovementSpeed(1.2f); /// 1.2 m/s
        _army1NPCs.push_back(soldier);
        _army1.push_back(aiSoldier);
    }

    soldierMesh = _sceneGraph->findNode("Soldier2");
    if(soldierMesh){
        soldierMesh->setSelectable(true);
        aiSoldier = New AIEntity(soldierMesh->getTransform()->getPosition(), "Soldier2");
        aiSoldier->addSensor(VISUAL_SENSOR,New VisualSensor());
        aiSoldier->setComInterface();
        aiSoldier->addActionProcessor(New WarSceneAIActionList());
        aiSoldier->setTeam(_faction2);
        NPC* soldier = New NPC(soldierMesh, aiSoldier);
        soldier->setMovementSpeed(1.23f); /// 1.23 m/s
        _army2NPCs.push_back(soldier);
        _army2.push_back(aiSoldier);
    }
    soldierMesh = _sceneGraph->findNode("Soldier3");
    if(soldierMesh){
        soldierMesh->setSelectable(true);
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

bool WarScene::loadResources(bool continueOnErrors){
    GUI::getInstance().addButton("Simulate", "Simulate", vec2<I32>(renderState().cachedResolution().width-220 ,
                                                                   renderState().cachedResolution().height/1.1f),
                                                         vec2<U32>(100,25),vec3<F32>(0.65f),
                                                         DELEGATE_BIND(&WarScene::startSimulation,this));

    GUI::getInstance().addText("fpsDisplay",           //Unique ID
                               vec2<I32>(60,60),          //Position
                                Font::DIVIDE_DEFAULT,    //Font
                               vec3<F32>(0.0f,0.2f, 1.0f),  //Color
                               "FPS: %s",0);    //Text and arguments
    GUI::getInstance().addText("RenderBinCount",
                                vec2<I32>(60,70),
                                 Font::DIVIDE_DEFAULT,
                                vec3<F32>(0.6f,0.2f,0.2f),
                                "Number of items in Render Bin: %d",0);

    GUI::getInstance().addText("camPosition",  vec2<I32>(60,100),
                                Font::DIVIDE_DEFAULT,
                                vec3<F32>(0.2f,0.8f,0.2f),
                                "Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ] [Pitch: %5.2f | Yaw: %5.2f]",
                                renderState().getCamera().getEye().x,
                                renderState().getCamera().getEye().y,
                                renderState().getCamera().getEye().z,
                                renderState().getCamera().getEuler().pitch,
                                renderState().getCamera().getEuler().yaw);

    GUI::getInstance().addText("lampPosition",  vec2<I32>(60,120),
                                Font::DIVIDE_DEFAULT,
                                vec3<F32>(0.2f,0.8f,0.2f),
                                "Lamp Position [ X: %5.0f | Y: %5.0f | Z: %5.0f ]",
                                0.0f, 0.0f, 0.0f);
    //Add a first person camera
    Camera* cam = New FirstPersonCamera();
    cam->setMoveSpeedFactor(0.2f);
    cam->setTurnSpeedFactor(0.1f);
    _cameraMgr->addNewCamera("fpsCamera", cam);
    //Add a third person camera
    cam = New ThirdPersonCamera();
    cam->setMoveSpeedFactor(0.02f);
    cam->setTurnSpeedFactor(0.01f);
    _cameraMgr->addNewCamera("tpsCamera", cam);

    _taskTimers.push_back(0.0); //Fps
    _taskTimers.push_back(0.0); //animation bull
    _taskTimers.push_back(0.0); //animation dwarf
    _taskTimers.push_back(0.0); //animation bob
    return true;
}

void WarScene::onKeyDown(const OIS::KeyEvent& key){
    Scene::onKeyDown(key);
    switch(key.key)	{
        default: break;
        case OIS::KC_W: state()._moveFB =  1; break;
        case OIS::KC_A:	state()._moveLR = -1; break;
        case OIS::KC_S:	state()._moveFB = -1; break;
        case OIS::KC_D:	state()._moveLR =  1; break;
        case OIS::KC_Q: state()._roll   = -1; break;
        case OIS::KC_E: state()._roll   =  1; break;
    }
}

void WarScene::onKeyUp(const OIS::KeyEvent& key){
    static bool fpsCameraActive = false;
    static bool tpsCameraActive = false;
    static bool flyCameraActive = true;

    Scene::onKeyUp(key);
    switch(key.key)	{
        default: break;
        case OIS::KC_W:
        case OIS::KC_S:	state()._moveFB = 0; break;
        case OIS::KC_A:
        case OIS::KC_D: state()._moveLR = 0; break;
        case OIS::KC_Q:
        case OIS::KC_E: state()._roll   = 0; break;
        case OIS::KC_F1: _sceneGraph->print(); break;
        case OIS::KC_TAB:{
            if(_currentSelection != NULL){
                /*if(flyCameraActive){
                    _cameraMgr->setActiveCamera("fpsCamera");
                    flyCameraActive = false; fpsCameraActive = true;
                }else if(fpsCameraActive){*/
                if(flyCameraActive){
                    _cameraMgr->setActiveCamera("tpsCamera");
                    /*fpsCameraActive*/flyCameraActive = false; tpsCameraActive = true;
                }else if(tpsCameraActive){
                    _cameraMgr->setActiveCamera("defaultCamera");
                    tpsCameraActive = false; flyCameraActive = true;
                }
            }
//			renderState().getCamera().setTargetNode(_currentSelection);
        }break;
    }
}
void WarScene::onMouseMove(const OIS::MouseEvent& key){
    if(_mousePressed[OIS::MB_Right]){
        if(_previousMousePos.x - key.state.X.abs > 1 )   	 state()._angleLR = -1;
        else if(_previousMousePos.x - key.state.X.abs < -1 ) state()._angleLR =  1;
        else 			                                     state()._angleLR =  0;

        if(_previousMousePos.y - key.state.Y.abs > 1 )		 state()._angleUD = -1;
        else if(_previousMousePos.y - key.state.Y.abs < -1 ) state()._angleUD =  1;
        else 	                                             state()._angleUD =  0;
    }

    Scene::onMouseMove(key);
}

void WarScene::onMouseClickDown(const OIS::MouseEvent& key, OIS::MouseButtonID button){
    Scene::onMouseClickDown(key,button);
}

void WarScene::onMouseClickUp(const OIS::MouseEvent& key, OIS::MouseButtonID button){
    Scene::onMouseClickUp(key,button);
    if(!_mousePressed[OIS::MB_Right]){
        state()._angleUD = 0;
        state()._angleLR = 0;
    }
}
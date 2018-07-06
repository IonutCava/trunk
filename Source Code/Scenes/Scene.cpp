#include "Headers/Scene.h"

#include <Hardware/Network/Headers/ASIOImpl.h>
#include "GUI/Headers/GUI.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/AIManager.h"
#include "Managers/Headers/SceneManager.h"
#include "Managers/Headers/CameraManager.h"
#include "Rendering/Camera/Headers/FreeFlyCamera.h"

#include "Environment/Sky/Headers/Sky.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"

#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Material/Headers/Material.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

#include "Dynamics/Physics/Headers/PhysicsSceneInterface.h"

Scene::Scene() :  Resource("temp_scene"),
                 _GFX(GFX_DEVICE),
                 _LRSpeedFactor(5.0f),
                 _loadComplete(false),
                 _cookCollisionMeshesScheduled(false),
                 _paramHandler(ParamHandler::getInstance()),
                 _currentSelection(nullptr),
                 _physicsInterface(nullptr),
                 _sceneGraph(New SceneGraph())
{
    _mousePressed[OIS::MB_Left]    = false;
    _mousePressed[OIS::MB_Right]   = false;
    _mousePressed[OIS::MB_Middle]  = false;
    _mousePressed[OIS::MB_Button3] = false;
    _mousePressed[OIS::MB_Button4] = false;
    _mousePressed[OIS::MB_Button5] = false;
    _mousePressed[OIS::MB_Button6] = false;
    _mousePressed[OIS::MB_Button7] = false;
}

Scene::~Scene()
{
}

bool Scene::idle(){ //Called when application is idle
    if(_sceneGraph){
        if(_sceneGraph->getRoot()->getChildren().empty()) return false;
        _sceneGraph->idle();
    }

    if(!_modelDataArray.empty())
        loadXMLAssets(true);

    if (_cookCollisionMeshesScheduled && checkLoadFlag()){
        if(GFX_DEVICE.getFrameCount() > 1){
            _sceneGraph->getRoot()->getComponent<PhysicsComponent>()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }
    return true;
}

void Scene::onCameraChange(){
    if(_sceneGraph)
        _sceneGraph->getRoot()->onCameraChange();
}

void Scene::postRender(){
#ifdef _DEBUG
    if(renderState()._debugDrawLines) {
        if (!_lines[DEBUG_LINE_RAY_PICK].empty()) {
            GFX_DEVICE.drawLines(_lines[DEBUG_LINE_OBJECT_TO_TARGET],  mat4<F32>(), vec4<I32>(), false,  false);
        }
    }
    if (!_lines[DEBUG_LINE_OBJECT_TO_TARGET].empty() && AI::AIManager::getInstance().navMeshDebugDraw()) {
        GFX_DEVICE.drawLines(_lines[DEBUG_LINE_OBJECT_TO_TARGET],  mat4<F32>(), vec4<I32>(), false,  false);
    }
#endif
}

void Scene::addPatch(vectorImpl<FileData>& data){
}

void Scene::loadXMLAssets(bool singleStep){
    while(!_modelDataArray.empty()){
        FileData& it = _modelDataArray.top();
        //vegetation is loaded elsewhere
        if(it.type == VEGETATION){
            _vegetationDataArray.push_back(it);
        }else{
            loadModel(it);
        }
        _modelDataArray.pop();
        
        if(singleStep){
            return;
        }
    }
}

bool Scene::loadModel(const FileData& data){
    if(data.type == PRIMITIVE)	
        return loadGeometry(data);

    ResourceDescriptor model(data.ModelName);
    model.setResourceLocation(data.ModelName);
    model.setFlag(true);
    Mesh *thisObj = CreateResource<Mesh>(model);
    if (!thisObj){
        ERROR_FN(Locale::get("ERROR_SCENE_LOAD_MODEL"),  data.ModelName.c_str());
        return false;
    }
    
    SceneGraphNode* meshNode = _sceneGraph->getRoot()->addNode(thisObj, data.ItemName);
    meshNode->castsShadows(data.castsShadows);
    meshNode->receivesShadows(data.receivesShadows);
    meshNode->getComponent<PhysicsComponent>()->setScale(data.scale);
    meshNode->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    meshNode->getComponent<PhysicsComponent>()->setPosition(data.position);
    if(data.staticUsage){
        meshNode->usageContext(SceneGraphNode::NODE_STATIC);
    }
    if(data.navigationUsage){
        meshNode->getComponent<NavigationComponent>()->navigationContext(NavigationComponent::NODE_OBSTACLE);
    }
    if(data.physicsUsage){
        meshNode->getComponent<PhysicsComponent>()->physicsGroup(data.physicsPushable ? PhysicsComponent::NODE_COLLIDE : PhysicsComponent::NODE_COLLIDE_NO_PUSH);
    }
    if(data.useHighDetailNavMesh){
        meshNode->getComponent<NavigationComponent>()->navigationDetailOverride(true);
    }
    return true;
}

bool Scene::loadGeometry(const FileData& data){
    Object3D* thisObj;
    ResourceDescriptor item(data.ItemName);
    item.setResourceLocation(data.ModelName);
    if(data.ModelName.compare("Box3D") == 0) {
            thisObj = CreateResource<Box3D>(item);
            dynamic_cast<Box3D*>(thisObj)->setSize(data.data);
    } else if(data.ModelName.compare("Sphere3D") == 0) {
            thisObj = CreateResource<Sphere3D>(item);
            dynamic_cast<Sphere3D*>(thisObj)->setRadius(data.data);
    } else if(data.ModelName.compare("Quad3D") == 0)	{
            vec3<F32> scale = data.scale;
            vec3<F32> position = data.position;
            P32 quadMask; quadMask.i = 0; quadMask.b.b0 = 1;
            item.setBoolMask(quadMask);
            thisObj = CreateResource<Quad3D>(item);
            dynamic_cast<Quad3D*>(thisObj)->setCorner(Quad3D::TOP_LEFT,vec3<F32>(0,1,0));
            dynamic_cast<Quad3D*>(thisObj)->setCorner(Quad3D::TOP_RIGHT,vec3<F32>(1,1,0));
            dynamic_cast<Quad3D*>(thisObj)->setCorner(Quad3D::BOTTOM_LEFT,vec3<F32>(0,0,0));
            dynamic_cast<Quad3D*>(thisObj)->setCorner(Quad3D::BOTTOM_RIGHT,vec3<F32>(1,0,0));
    } else if(data.ModelName.compare("Text3D") == 0) {
            ///set font file
            item.setResourceLocation(data.data3);
            item.setPropertyList(data.data2);
            thisObj = CreateResource<Text3D>(item);
            dynamic_cast<Text3D*>(thisObj)->getWidth() = data.data;
    }else{
        ERROR_FN(Locale::get("ERROR_SCENE_UNSUPPORTED_GEOM"),data.ModelName.c_str());
        return false;
    }
    Material* tempMaterial = XML::loadMaterial(data.ItemName+"_material");
    if(!tempMaterial){
        ResourceDescriptor materialDescriptor(data.ItemName+"_material");
        tempMaterial = CreateResource<Material>(materialDescriptor);
        tempMaterial->setDiffuse(data.color);
    }

    thisObj->setMaterial(tempMaterial);
    SceneGraphNode* thisObjSGN = _sceneGraph->getRoot()->addNode(thisObj);
    thisObjSGN->getComponent<PhysicsComponent>()->setScale(data.scale);
    thisObjSGN->getComponent<PhysicsComponent>()->setRotation(data.orientation);
    thisObjSGN->getComponent<PhysicsComponent>()->setPosition(data.position);
    thisObjSGN->castsShadows(data.castsShadows);
    thisObjSGN->receivesShadows(data.receivesShadows);
    if(data.staticUsage){
        thisObjSGN->usageContext(SceneGraphNode::NODE_STATIC);
    }
    if(data.navigationUsage){
        thisObjSGN->getComponent<NavigationComponent>()->navigationContext(NavigationComponent::NODE_OBSTACLE);
    }
    if(data.physicsUsage){
        thisObjSGN->getComponent<PhysicsComponent>()->physicsGroup(data.physicsPushable ? PhysicsComponent::NODE_COLLIDE : PhysicsComponent::NODE_COLLIDE_NO_PUSH);
    }
    if(data.useHighDetailNavMesh){
        thisObjSGN->getComponent<NavigationComponent>()->navigationDetailOverride(true);
    }
    return true;
}

ParticleEmitter* Scene::getParticleEmitter(const std::string& name){
    ParticleEmitterMap::const_iterator emitterIt = _particleEmitters.find(name);
    if(emitterIt != _particleEmitters.end()){
        return _particleEmitters[name];
    }
    return nullptr;
}

ParticleEmitter* Scene::addParticleEmitter(const std::string& name, const ParticleEmitterDescriptor& descriptor){
   ParticleEmitterMap::const_iterator emitterIt = _particleEmitters.find(name);
   if(emitterIt != _particleEmitters.end()){
       RemoveResource(_particleEmitters[name]);
   }

   ResourceDescriptor particleEmitter(name);
   ParticleEmitter* temp = CreateResource<ParticleEmitter>(particleEmitter);    
   _particleEmitters[name] = temp;
   temp->setDescriptor(descriptor);
   return temp;
}

SceneGraphNode* Scene::addLight(Light* const lightItem, SceneGraphNode* const parentNode){
    SceneGraphNode* returnNode = nullptr;
    if(parentNode)
        returnNode = parentNode->addNode(lightItem);
    else
        returnNode = _sceneGraph->getRoot()->addNode(lightItem);

    return returnNode;
}

DirectionalLight* Scene::addDefaultLight(){
    std::stringstream ss; ss << LightManager::getInstance().getLights().size();
    ResourceDescriptor defaultLight("Default directional light "+ss.str());
    defaultLight.setId(0); //descriptor ID is not the same as light ID. This is the light's slot!!
    defaultLight.setResourceLocation("root");
    defaultLight.setEnumValue(LIGHT_TYPE_DIRECTIONAL);
    DirectionalLight* l = dynamic_cast<DirectionalLight*>(CreateResource<Light>(defaultLight));
    l->setCastShadows(true);
    addLight(l);
    vec3<F32> ambientColor(0.1f, 0.1f, 0.1f);
    LightManager::getInstance().setAmbientLight(ambientColor);
    return l;
}
///Add skies
Sky* Scene::addDefaultSky(){
    STUBBED("ToDo: load skyboxes from XML")
    Sky* tempSky = New Sky("Default Sky");
    _skiesSGN.push_back(_sceneGraph->getRoot()->addNode(tempSky));
    return tempSky;
}

SceneGraphNode* Scene::addGeometry(SceneNode* const object,const std::string& sgnName){
    return _sceneGraph->getRoot()->addNode(object,sgnName);
}

bool Scene::removeGeometry(SceneNode* node){
    if(!node) {
        ERROR_FN(Locale::get("ERROR_SCENE_DELETE_NULL_NODE"));
        return false;
    }
    SceneGraphNode* _graphNode = _sceneGraph->findNode(node->getName());

    SAFE_DELETE_CHECK(_graphNode);
}

bool Scene::preLoad() {
    _GFX.enableFog(_sceneState.getFogDesc()._fogDensity,
                   _sceneState.getFogDesc()._fogColor);
    return true;
}

bool Scene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const guiInterface){
    _GUI = guiInterface;
    _name = name;
    renderState()._cameraMgr = cameraMgr;
    preLoad();
    loadXMLAssets();
    SceneGraphNode* root = _sceneGraph->getRoot();
    //Add terrain from XML
    if(!_terrainInfoArray.empty()){
        for(U8 i = 0; i < _terrainInfoArray.size(); i++){
            ResourceDescriptor terrain(_terrainInfoArray[i]->getVariable("terrainName"));
            Terrain* temp = CreateResource<Terrain>(terrain);
            SceneGraphNode* terrainTemp = root->addNode(temp);
            terrainTemp->setActive(_terrainInfoArray[i]->getActive());
            terrainTemp->usageContext(SceneGraphNode::NODE_STATIC);
            terrainTemp->getComponent<NavigationComponent>()->navigationContext(NavigationComponent::NODE_OBSTACLE);
            terrainTemp->getComponent<PhysicsComponent>()->physicsGroup(_terrainInfoArray[i]->getCreatePXActor() ? PhysicsComponent::NODE_COLLIDE_NO_PUSH : PhysicsComponent::NODE_COLLIDE_IGNORE);
        }
    }
    //Camera position is overridden in the scene's XML configuration file
    if(ParamHandler::getInstance().getParam<bool>("options.cameraStartPositionOverride")){
        renderState().getCamera().setEye(vec3<F32>(_paramHandler.getParam<F32>("options.cameraStartPosition.x"),
                                                     _paramHandler.getParam<F32>("options.cameraStartPosition.y"),
                                                     _paramHandler.getParam<F32>("options.cameraStartPosition.z")));
        vec2<F32> camOrientation(_paramHandler.getParam<F32>("options.cameraStartOrientation.xOffsetDegrees"),
                                 _paramHandler.getParam<F32>("options.cameraStartOrientation.yOffsetDegrees"));
        renderState().getCamera().setGlobalRotation(camOrientation.y/*yaw*/,camOrientation.x/*pitch*/);
    }else{
        renderState().getCamera().setEye(vec3<F32>(0,50,0));
    }

    //Create an AI thread, but start it only if needed
    Kernel* kernel = Application::getInstance().getKernel();
    _aiTask.reset(kernel->AddTask(1000.0 / Config::AI_THREAD_UPDATE_FREQUENCY, false, false,
                                  DELEGATE_BIND(&AI::AIManager::update, DELEGATE_REF(AI::AIManager::getInstance()))));

    addSelectionCallback(DELEGATE_BIND(&GUI::selectionChangeCallback, DELEGATE_REF(GUI::getInstance()), this));
    _loadComplete = true;
    return _loadComplete;
}

bool Scene::unload(){
    // prevent double unload calls
    if(!checkLoadFlag()) return false;
    clearTasks();
    clearObjects();
    clearLights();
    clearPhysics();
    _loadComplete = false;
    return true;
}

PhysicsSceneInterface* Scene::createPhysicsImplementation(){
    return PHYSICS_DEVICE.NewSceneInterface(this);
}

bool Scene::loadPhysics(bool continueOnErrors){
    //Add a new physics scene (can be overridden in each scene for custom behavior)
    _physicsInterface = createPhysicsImplementation();
    PHYSICS_DEVICE.setPhysicsScene(_physicsInterface);
    //Initialize the physics scene
    PHYSICS_DEVICE.initScene();
    //Cook geometry
    if(_paramHandler.getParam<bool>("options.autoCookPhysicsAssets"))
        _cookCollisionMeshesScheduled = true;
    return true;
}

void Scene::clearPhysics(){
    if(_physicsInterface){
        _physicsInterface->exit();
        SAFE_DELETE(_physicsInterface);
    }
}

bool Scene::initializeAI(bool continueOnErrors)   {
    _aiTask->startTask();
    return true;
}

bool Scene::deinitializeAI(bool continueOnErrors) {	///Shut down AIManager thread
    if(_aiTask.get()){
        _aiTask->stopTask();
        _aiTask.reset();
    }
    while(_aiTask.get()){};
    return true;
}

void Scene::clearObjects(){
    for(U8 i = 0; i < _terrainInfoArray.size(); ++i){
        RemoveResource(_terrainInfoArray[i]);
    }
    _skiesSGN.clear(); //< Skies are cleared in the SceneGraph
    _terrainInfoArray.clear();
    while(!_modelDataArray.empty())
        _modelDataArray.pop();
    _vegetationDataArray.clear();

    _particleEmitters.clear();
    SAFE_DELETE(_sceneGraph);
}

void Scene::clearLights(){
    LightManager::getInstance().clear();
}

bool Scene::updateCameraControls(){

    Camera& cam = renderState().getCamera();
    switch (cam.getType()){
        default:
        case Camera::FREE_FLY:{
            if (state()._angleLR) cam.rotateYaw(state()._angleLR);
            if (state()._angleUD) cam.rotatePitch(state()._angleUD);
            if (state()._roll)    cam.rotateRoll(state()._roll);
            if (state()._moveFB)  cam.moveForward(state()._moveFB);
            if (state()._moveLR)  cam.moveStrafe(state()._moveLR);
        }break;
    }

    state()._cameraUpdated =  (state()._moveFB || state()._moveLR || state()._angleLR || state()._angleUD || state()._roll);
    return state()._cameraUpdated;
}

void Scene::updateSceneState(const U64 deltaTime){
    updateSceneStateInternal(deltaTime);
    state()._cameraUnderwater = renderState().getCamera().getEye().y < state()._waterHeight;
    _sceneGraph->sceneUpdate(deltaTime, _sceneState);
}

void Scene::deleteSelection(){
    if(_currentSelection != nullptr){
        _currentSelection->scheduleDeletion();
    }
}

void Scene::addTask(Task_ptr taskItem) {
    _tasks.push_back(taskItem);
}

void Scene::clearTasks(){
    PRINT_FN(Locale::get("STOP_SCENE_TASKS"));
    for(Task_ptr& task : _tasks){
        task->stopTask();
    }
    _tasks.clear();
}

void Scene::removeTask(Task_ptr taskItem){
    taskItem->stopTask();

    for(vectorImpl<Task_ptr>::iterator it = _tasks.begin(); it != _tasks.end(); it++){
        if((*it)->getGUID() == taskItem->getGUID()){
            _tasks.erase(it);
            return;
        }
    }
}

void Scene::removeTask(U32 guid){
    for(vectorImpl<Task_ptr>::iterator it = _tasks.begin(); it != _tasks.end(); it++){
        if((*it)->getGUID() == guid){
            (*it)->stopTask();
            _tasks.erase(it);
            return;
        }
    }
}

void Scene::processGUI(const U64 deltaTime){
    for (U16 i = 0; i < _guiTimers.size(); ++i)
        _guiTimers[i] += getUsToMs(deltaTime);
}

void Scene::processTasks(const U64 deltaTime){
    for(U16 i = 0; i < _taskTimers.size(); ++i)
        _taskTimers[i] += getUsToMs(deltaTime);
}

TerrainDescriptor* Scene::getTerrainInfo(const std::string& terrainName) {
    for (U8 i = 0; i < _terrainInfoArray.size(); i++)
    if (terrainName.compare(_terrainInfoArray[i]->getVariable("terrainName")) == 0)
        return _terrainInfoArray[i];

    DIVIDE_ASSERT(false, "Scene error: INVALID TERRAIN NAME FOR INFO LOOKUP"); // not found;
    return _terrainInfoArray[0];
}

void Scene::debugDraw(const RenderStage& stage) {
#ifdef _DEBUG
    const SceneRenderState::GizmoState& currentGizmoState = renderState().gizmoState();

    GFX_DEVICE.drawDebugAxis(currentGizmoState != SceneRenderState::NO_GIZMO);

    if (currentGizmoState == SceneRenderState::SELECTED_GIZMO) {
        if (_currentSelection != nullptr) {
            _currentSelection->drawDebugAxis();
        }
    } 
#endif
    if (GFX_DEVICE.isCurrentRenderStage(DISPLAY_STAGE)) {
        // Draw bounding boxes, skeletons, axis gizmo, etc.
        GFX_DEVICE.debugDraw(renderState());
        // Show NavMeshes
        AI::AIManager::getInstance().debugDraw(false);
    }
}
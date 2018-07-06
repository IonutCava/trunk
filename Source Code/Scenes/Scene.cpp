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

Scene::Scene() :  Resource(),
                 _GFX(GFX_DEVICE),
                 _FBSpeedFactor(1.0f),
                 _LRSpeedFactor(5.0f),
                 _loadComplete(false),
                 _cookCollisionMeshesScheduled(false),
                 _paramHandler(ParamHandler::getInstance()),
                 _currentSelection(NULL),
                 _physicsInterface(NULL),
                 _cameraMgr(NULL),
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

    if(_cookCollisionMeshesScheduled){
        if(SceneManager::getInstance().getFrameCount() > 1){
            _sceneGraph->getRoot()->cookCollisionMesh(_name);
            _cookCollisionMeshesScheduled = false;
        }
    }
    return true;
}

void Scene::updateCameras(){
    renderState().updateCamera(_cameraMgr->getActiveCamera());
}

void Scene::postRender(){
#ifdef _DEBUG
    if(renderState()._debugDrawLines) {
        for(U8 i = 0; i < DEBUG_LINE_PLACEHOLDER; ++i)
            if(!_pointsA[i].empty())
                GFX_DEVICE.drawLines(_pointsA[i], _pointsB[i], _colors[i], mat4<F32>(),  false,  false);
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

    meshNode->getTransform()->scale(data.scale);
    meshNode->getTransform()->rotateEuler(data.orientation);
    meshNode->getTransform()->translate(data.position);
    if(data.staticUsage){
        meshNode->setUsageContext(SceneGraphNode::NODE_STATIC);
    }
    if(data.navigationUsage){
        meshNode->setNavigationContext(SceneGraphNode::NODE_OBSTACLE);
    }
    if(data.physicsUsage){
        meshNode->setPhysicsGroup(data.physicsPushable ? SceneGraphNode::NODE_COLLIDE : SceneGraphNode::NODE_COLLIDE_NO_PUSH);
    }
    if(data.useHighDetailNavMesh){
        meshNode->setNavigationDetailOverride(true);
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
    thisObjSGN->getTransform()->scale(data.scale);
    thisObjSGN->getTransform()->rotateEuler(data.orientation);
    thisObjSGN->getTransform()->translate(data.position);
    if(data.staticUsage){
        thisObjSGN->setUsageContext(SceneGraphNode::NODE_STATIC);
    }
    if(data.navigationUsage){
        thisObjSGN->setNavigationContext(SceneGraphNode::NODE_OBSTACLE);
    }
    if(data.physicsUsage){
        thisObjSGN->setPhysicsGroup(data.physicsPushable ? SceneGraphNode::NODE_COLLIDE : SceneGraphNode::NODE_COLLIDE_NO_PUSH);
    }
    if(data.useHighDetailNavMesh){
        thisObjSGN->setNavigationDetailOverride(true);
    }
    return true;
}

SceneGraphNode* Scene::addLight(Light* const lightItem, SceneGraphNode* const parentNode){
    SceneGraphNode* returnNode = NULL;
    if(parentNode)
        returnNode = parentNode->addNode(lightItem);
    else
        returnNode = _sceneGraph->getRoot()->addNode(lightItem);

    return returnNode;
}

Camera* Scene::addDefaultCamera(){
    PRINT_FN(Locale::get("SCENE_ADD_DEFAULT_CAMERA"), getName().c_str());
    Camera* camera = New FreeFlyCamera();
    camera->setMoveSpeedFactor(_paramHandler.getParam<F32>("options.cameraSpeed.move"));
    camera->setTurnSpeedFactor(_paramHandler.getParam<F32>("options.cameraSpeed.turn"));
    camera->setFixedYawAxis(true);
    //As soon as a camera is added to the camera manager, the manager is responsible for cleaning it up
    _cameraMgr->addNewCamera("defaultCamera", camera);
    _cameraMgr->setActiveCamera("defaultCamera");
    return camera;
}

Light* Scene::addDefaultLight(){
    std::stringstream ss; ss << LightManager::getInstance().getLights().size();
    ResourceDescriptor defaultLight("Default directional light "+ss.str());
    defaultLight.setId(0); //descriptor ID is not the same as light ID. This is the light's slot!!
    defaultLight.setResourceLocation("root");
    defaultLight.setEnumValue(LIGHT_TYPE_DIRECTIONAL);
    Light* l = CreateResource<Light>(defaultLight);
    addLight(l);
    vec4<F32> ambientColor(0.1f, 0.1f, 0.1f, 1.0f);
    LightManager::getInstance().setAmbientLight(ambientColor);
    return l;
}
///Add skies
#pragma message("ToDo: load skyboxes from XML")
Sky* Scene::addDefaultSky(){
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
    _GFX.enableFog(_sceneState.getFogDesc()._fogMode,
                   _sceneState.getFogDesc()._fogDensity,
                   _sceneState.getFogDesc()._fogColor,
                   _sceneState.getFogDesc()._fogStartDist,
                   _sceneState.getFogDesc()._fogEndDist);
    renderState().shadowMapResolutionFactor(_paramHandler.getParam<U8>("rendering.shadowResolutionFactor"));
    return true;
}

bool Scene::load(const std::string& name, CameraManager* const cameraMgr, GUI* const guiInterface){
    _GUI = guiInterface;
    _cameraMgr = cameraMgr;
    _name = name;
    addDefaultCamera();
    preLoad();
    loadXMLAssets();
    SceneGraphNode* root = _sceneGraph->getRoot();
    //Add terrain from XML
    if(!_terrainInfoArray.empty()){
        for(U8 i = 0; i < _terrainInfoArray.size(); i++){
            ResourceDescriptor terrain(_terrainInfoArray[i]->getVariable("terrainName"));
            Terrain* temp = CreateResource<Terrain>(terrain);
            SceneGraphNode* terrainTemp = root->addNode(temp);
            terrainTemp->useDefaultTransform(false);
            terrainTemp->setTransform(NULL);
            terrainTemp->setActive(_terrainInfoArray[i]->getActive());
            terrainTemp->setUsageContext(SceneGraphNode::NODE_STATIC);
            terrainTemp->setNavigationContext(SceneGraphNode::NODE_OBSTACLE);
            terrainTemp->setPhysicsGroup(_terrainInfoArray[i]->getCreatePXActor() ? SceneGraphNode::NODE_COLLIDE_NO_PUSH : SceneGraphNode::NODE_COLLIDE_IGNORE);
            temp->initializeVegetation(_terrainInfoArray[i],terrainTemp);
        }
    }
    //Camera position is overriden in the scene's XML configuration file
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
    _aiTask.reset(New Task(kernel->getThreadPool(),
                           1000.0 / Config::AI_THREAD_UPDATE_FREQUENCY,
                           false,
                           false,
                           DELEGATE_BIND(&AIManager::update,
                           DELEGATE_REF(AIManager::getInstance()))));
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
    //Add a new physics scene (can be overriden in each scene for custom behaviour)
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
        delete _physicsInterface;
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
    for(U8 i = 0; i < _terrainInfoArray.size(); i++){
        RemoveResource(_terrainInfoArray[i]);
    }
    _skiesSGN.clear(); //< Skies are cleared in the SceneGraph
    _terrainInfoArray.clear();
    while(!_modelDataArray.empty())
        _modelDataArray.pop();
    _vegetationDataArray.clear();
    assert(_sceneGraph);

    SAFE_DELETE(_sceneGraph);
}

void Scene::clearLights(){
    LightManager::getInstance().clear();
}

void Scene::updateSceneState(const U64 deltaTime){
    _sceneGraph->sceneUpdate(deltaTime, _sceneState);
}

void Scene::deleteSelection(){
    if(_currentSelection != NULL){
        _currentSelection->scheduleDeletion();
    }
}

void Scene::addTask(Task_ptr taskItem) {
    _tasks.push_back(taskItem);
}

void Scene::clearTasks(){
    PRINT_FN(Locale::get("STOP_SCENE_TASKS"));
    for_each(Task_ptr& task, _tasks){
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

void Scene::processTasks(const U64 deltaTime){
    for(U16 i = 0; i < _taskTimers.size(); ++i)
        _taskTimers[i] += getUsToMs(deltaTime);
}
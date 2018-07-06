#include "Headers/Scene.h"

#include <Hardware/Network/Headers/ASIOImpl.h>
#include "GUI/Headers/GUI.h"

#include "Core/Headers/ParamHandler.h"
#include "Core/Math/Headers/Transform.h"

#include "Utility/Headers/XMLParser.h"
#include "Rendering/Headers/Frustum.h"
#include "Managers/Headers/AIManager.h"

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
			   _paramHandler(ParamHandler::getInstance()),
			   _currentSelection(NULL),
 			   _physicsInterface(NULL),
			   _sceneGraph(New SceneGraph()),
			   _sceneState(New SceneState()),
			   _sceneRenderState(New SceneRenderState())
{
}

Scene::~Scene() {
	SAFE_DELETE(_sceneState);
	SAFE_DELETE(_sceneRenderState);
};

bool Scene::idle(){ //Called when application is idle
	if(_sceneGraph){
		if(_sceneGraph->getRoot()->getChildren().empty()) return false;
		_sceneGraph->idle();
	}

	bool _updated = false;
	if(!_pendingDataArray.empty())
	for(vectorImpl<FileData>::iterator iter = _pendingDataArray.begin(); iter != _pendingDataArray.end(); ++iter) {
		if(!loadModel(*iter)){
			WorldPacket p(CMSG_REQUEST_GEOMETRY);
			p << (*iter).ModelName;
			ASIOImpl::getInstance().sendPacket(p);
			while(!loadModel(*iter)){
				PRINT_FN(Locale::get("AWAITING_FILE"));
			}
		}else{
			vectorImpl<FileData>::iterator iter2;
			for(iter2 = _modelDataArray.begin(); iter2 != _modelDataArray.end(); ++iter2)	{
				if((*iter2).ItemName.compare((*iter).ItemName) == 0){
					_modelDataArray.erase(iter2);
					_modelDataArray.push_back(*iter);
					_pendingDataArray.erase(iter);
					_updated = true;
					break;
				}
			}
			if(_updated) break;
		}
	}
	return true;
}

void Scene::postRender(){
	if(GFX_DEVICE.isCurrentRenderStage(FINAL_STAGE) ){
		// Draw bounding boxes, skeletons, axis gizmo, etc.
		_GFX.debugDraw();
		// Preview depthmaps if needed
		LightManager::getInstance().previewShadowMaps();
		// Show navmeshes
		AIManager::getInstance().debugDraw(false);
	}
}

void Scene::addPatch(vectorImpl<FileData>& data){
}

void Scene::loadXMLAssets(){
	for(vectorImpl<FileData>::iterator it = _modelDataArray.begin(); it != _modelDataArray.end();){
		//vegetation is loaded elsewhere
		if((*it).type == VEGETATION){
			_vegetationDataArray.push_back(*it);
			it = _modelDataArray.erase(it);
		}else{
			loadModel(*it);
			++it;
		}
	}
}

bool Scene::loadModel(const FileData& data){
	if(data.type == PRIMITIVE)	return loadGeometry(data);

	ResourceDescriptor model(data.ItemName);
	model.setResourceLocation(data.ModelName);
	model.setFlag(true);
	Mesh *thisObj = CreateResource<Mesh>(model);
	if (!thisObj){
		ERROR_FN(Locale::get("ERROR_SCENE_LOAD_MODEL"),  data.ModelName.c_str());
		return false;
	}
	SceneGraphNode* meshNode = _sceneGraph->getRoot()->addNode(thisObj);

	meshNode->getTransform()->scale(data.scale);
	meshNode->getTransform()->rotateEuler(data.orientation);
	meshNode->getTransform()->translate(data.position);
    if(data.staticUsage){
        meshNode->setUsageContext(SceneGraphNode::NODE_STATIC);
    }
    if(data.navigationUsage){
        meshNode->setNavigationContext(SceneGraphNode::NODE_OBSTACLE);
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
	return true;
}

void Scene::addLight(Light* const lightItem){
	_sceneGraph->getRoot()->addNode(lightItem);
	LightManager::getInstance().addLight(lightItem);
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
Sky* Scene::addDefaultSky(){
#pragma message("ToDo: load skyboxes from XML")
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
    ParamHandler& par = ParamHandler::getInstance();
    _GFX.enableFog(_sceneState->getFogDesc()._fogMode,
                   _sceneState->getFogDesc()._fogDensity,
                   _sceneState->getFogDesc()._fogColor,
                   _sceneState->getFogDesc()._fogStartDist,
                   _sceneState->getFogDesc()._fogEndDist);
	_sceneRenderState->shadowMapResolutionFactor() = par.getParam<U8>("rendering.shadowResolutionFactor");
	return true;
}

bool Scene::load(const std::string& name){
	SceneGraphNode* root = _sceneGraph->getRoot();
	///Add terrain from XML
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
			temp->initializeVegetation(_terrainInfoArray[i],terrainTemp);
		}
	}
	///Camera position is overriden in the scene's XML configuration file
	if(ParamHandler::getInstance().getParam<bool>("options.cameraStartPositionOverride")){
		renderState()->getCamera()->setEye(vec3<F32>(_paramHandler.getParam<F32>("options.cameraStartPosition.x"),
													 _paramHandler.getParam<F32>("options.cameraStartPosition.y"),
													 _paramHandler.getParam<F32>("options.cameraStartPosition.z")));
		vec2<F32> camOrientation(_paramHandler.getParam<F32>("options.cameraStartOrientation.xOffsetDegrees"),
								 _paramHandler.getParam<F32>("options.cameraStartOrientation.yOffsetDegrees"));
		renderState()->getCamera()->setAngleX(RADIANS(camOrientation.x));
		renderState()->getCamera()->setAngleY(RADIANS(camOrientation.y));
	}else{
		renderState()->getCamera()->setEye(vec3<F32>(0,50,0));
	}

	///Create an AI thread, but start it only if needed
	Kernel* kernel = Application::getInstance().getKernel();
	_aiTask.reset(New Task(kernel->getThreadPool(),10,false,false,DELEGATE_BIND(&AIManager::tick, DELEGATE_REF(AIManager::getInstance()))));
	_loadComplete = true;
	return _loadComplete;
}

bool Scene::unload(){
	clearTasks();
	clearObjects();
	clearLights();
    clearPhysics();
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
	_skiesSGN.clear(); ///< Skies are cleared in the SceneGraph
	_terrainInfoArray.clear();
	_modelDataArray.clear();
	_vegetationDataArray.clear();
	_pendingDataArray.clear();
	assert(_sceneGraph);

	SAFE_DELETE(_sceneGraph);
}

void Scene::clearLights(){
	LightManager::getInstance().clear();
}

void Scene::clearTasks(){
	PRINT_FN(Locale::get("STOP_SCENE_TASKS"));
    removeTasks();
}

void Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
}

void Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){
}

static F32 speedFactor = 0.25f;
void Scene::onKeyDown(const OIS::KeyEvent& key){
	switch(key.key){
		case OIS::KC_LEFT :
			state()->_angleLR = -(speedFactor/2.5);
			break;
		case OIS::KC_RIGHT :
			state()->_angleLR = speedFactor/2.5;
			break;
		case OIS::KC_UP :
			state()->_angleUD = -(speedFactor/5);
			break;
		case OIS::KC_DOWN :
			state()->_angleUD = speedFactor/5;
			break;
		case OIS::KC_END:
			deleteSelection();
			break;
		case OIS::KC_F2:{
			D_PRINT_FN(Locale::get("TOGGLE_SCENE_SKELETONS"));
			renderState()->toggleSkeletons();
			}break;
		case OIS::KC_B:{
			D_PRINT_FN(Locale::get("TOGGLE_SCENE_BOUNDING_BOXES"));
			renderState()->toggleBoundingBoxes();
			}break;
		case OIS::KC_ADD:
			if (speedFactor < 10)  speedFactor += 0.1f;
			break;
		case OIS::KC_SUBTRACT:
			if (speedFactor > 0.1f)   speedFactor -= 0.1f;
			break;
		case OIS::KC_F8:
			ParamHandler::getInstance().setParam("postProcessing.enablePostFX",!ParamHandler::getInstance().getParam<bool>("postProcessing.enablePostFX"));
			break;
		case OIS::KC_F10:
			LightManager::getInstance().togglePreviewShadowMaps();
			break;
		case OIS::KC_F12:
			GFX_DEVICE.Screenshot("screenshot_",vec4<F32>(0,0,renderState()->_cachedResolution.x,renderState()->_cachedResolution.y));
			break;
		default:
			break;
	}
}

void Scene::onKeyUp(const OIS::KeyEvent& key){
	switch( key.key ){
		case OIS::KC_LEFT:
		case OIS::KC_RIGHT:
			state()->_angleLR = 0.0f;
			break;
		case OIS::KC_UP:
		case OIS::KC_DOWN:
			state()->_angleUD = 0.0f;
			break;
		case OIS::KC_F3:
			_paramHandler.setParam("postProcessing.enableDepthOfField", !_paramHandler.getParam<bool>("postProcessing.enableDepthOfField"));
			break;
        case OIS::KC_F4:
            _paramHandler.setParam("postProcessing.enableBloom", !_paramHandler.getParam<bool>("postProcessing.enableBloom"));
            break;
        case OIS::KC_F5:
            GFX_DEVICE.drawDebugAxis(!GFX_DEVICE.drawDebugAxis());
            break;
		default:
			break;
	}
}

void Scene::onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone){
	if(key.device->getID() != InputInterface::JOY_1) return;
	if(axis == 1){
		if(key.state.mAxes[axis].abs > deadZone)
			state()->_angleLR = speedFactor/5;
		else if(key.state.mAxes[axis].abs < -deadZone)
			state()->_angleLR = -(speedFactor/5);
		else
			state()->_angleLR = 0;
	}else if(axis == 0){
		if(key.state.mAxes[axis].abs > deadZone)
			state()->_angleUD = speedFactor/5;
		else if(key.state.mAxes[axis].abs < -deadZone)
			state()->_angleUD = -(speedFactor/5);
		else
			state()->_angleUD = 0;
	}else if(axis == 2){
		if(key.state.mAxes[axis].abs < -deadZone)
			state()->_moveFB = 0.25f;
		else if(key.state.mAxes[axis].abs > deadZone)
			state()->_moveFB = -0.25f;
		else
			state()->_moveFB = 0;
	}else if(axis == 3){
		if(key.state.mAxes[axis].abs < -deadZone)
			state()->_moveLR = 0.25f;
		else if(key.state.mAxes[axis].abs > deadZone)
			state()->_moveLR = -0.25f;
		else
			state()->_moveLR = 0;
	}
}

void Scene::updateSceneState(const U32 sceneTime){
	_sceneGraph->sceneUpdate(sceneTime);
}

void Scene::findSelection(const vec3<F32>& camOrigin, U32 x, U32 y){
    F32 value_fov = 0.7853f;    //this is 45 degrees converted to radians
    F32 value_aspect = _paramHandler.getParam<F32>("runtime.aspectRatio");
	F32 half_resolution_width = renderState()->_cachedResolution.width / 2.0f;
	F32 half_resolution_height = renderState()->_cachedResolution.height / 2.0f;

    F32 modifier_x, modifier_y;
        //mathematical handling of the difference between
        //your mouse position and the 'center' of the window

    vec3<F32> point;
        //the untransformed ray will be put here

    F32 point_dist = _paramHandler.getParam<F32>("runtime.zFar");
        //it'll be put this far on the Z plane

    vec3<F32> camera_origin;
        //this is where the camera sits, in 3dspace

    vec3<F32> point_xformed;
        //this is the transformed point

    vec3<F32> final_point;
    vec4<F32> color(0.0, 1.0, 0.0, 1.0);

    //These lines are the biggest part of this function.
    //This is where the mouse position is turned into a mathematical
    //'relative' of 3d space. The conversion to an actual point
    modifier_x = (F32)tan( value_fov * 0.5f )
        * (( 1.0f - x / half_resolution_width ) * ( value_aspect ) );
    modifier_y = (F32)tan( value_fov * 0.5f )
        * -( 1.0f - y / half_resolution_height );

    //These 3 take our modifier_x/y values and our 'casting' distance
    //to throw out a point in space that lies on the point_dist plane.
    //If we were using completely untransformed, untranslated space,
    //this would be fine - but we're not :)
    point.x = modifier_x * point_dist;
    point.y = modifier_y * point_dist;
    point.z = point_dist;

    //Next we make an openGL call to grab our MODELVIEW_MATRIX -
    //This is the matrix that rasters 3d points to 2d space - which is
    //kinda what we're doing, in reverse
    mat4<GLfloat> temp;
    GFX_DEVICE.getMatrix(MV_MATRIX,temp);
    //Some folks would then invert the matrix - I invert the results.

    //First, to get the camera_origin, we transform the 12, 13, 14
    //slots of our pulledMatrix - this gets us the actual viewing
    //position we are 'sitting' at when the function is called
    camera_origin.x = -(
        temp.mat[0] * temp.mat[12] +
        temp.mat[1] * temp.mat[13] +
        temp.mat[2] * temp.mat[14]);
    camera_origin.y = -(
        temp.mat[4] * temp.mat[12] +
        temp.mat[5] * temp.mat[13] +
        temp.mat[6] * temp.mat[14]);
    camera_origin.z = -(
        temp.mat[8] * temp.mat[12] +
        temp.mat[9] * temp.mat[13] +
        temp.mat[10] * temp.mat[14]);

    //Second, we transform the position we generated earlier - the '3d'
    //mouse position - by our viewing matrix.
    point_xformed.x = -(
        temp.mat[0] * point[0] +
        temp.mat[1] * point[1] +
        temp.mat[2] * point[2]);
    point_xformed.y = -(
        temp.mat[4] * point[0] +
        temp.mat[5] * point[1] +
        temp.mat[6] * point[2]);
    point_xformed.z = -(
        temp.mat[8] * point[0] +
        temp.mat[9] * point[1] +
        temp.mat[10] * point[2]);

    final_point = point_xformed + camera_origin;

	vec3<F32> origin(camOrigin);
	vec3<F32> dir = origin.direction(final_point);

	Ray r(origin,dir);
	_currentSelection = _sceneGraph->Intersect(r,Frustum::getInstance().getZPlanes().x,
												 Frustum::getInstance().getZPlanes().y/2.0f);
}

void Scene::deleteSelection(){
	if(_currentSelection != NULL){
		_currentSelection->scheduleDeletion();
	}
}

void Scene::removeTasks(){
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

void Scene::addTask(Task_ptr taskItem) {
	_tasks.push_back(taskItem);
}
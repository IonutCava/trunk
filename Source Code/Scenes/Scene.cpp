#include "Headers/Scene.h"

#include "ASIO.h"
#include "GUI/Headers/GUI.h"
#include "Utility/Headers/Guardian.h"
#include "Utility/Headers/XMLParser.h"
#include "Managers/Headers/SceneManager.h" //Object selection
#include "Managers/Headers/AIManager.h"
#include "Environment/Terrain/Headers/Terrain.h"
#include "Environment/Terrain/Headers/TerrainDescriptor.h"
#include "Geometry/Shapes/Headers/Mesh.h"
#include "Geometry/Shapes/Headers/Predefined/Box3D.h"
#include "Geometry/Shapes/Headers/Predefined/Quad3D.h"
#include "Geometry/Shapes/Headers/Predefined/Sphere3D.h"
#include "Geometry/Shapes/Headers/Predefined/Text3D.h"

using namespace std;


bool Scene::clean(){ //Called when application is idle
	if(_sceneGraph){
		if(_sceneGraph->getRoot()->getChildren().empty()) return false;
	}

	bool _updated = false;
	if(!PendingDataArray.empty())
	for(vector<FileData>::iterator iter = PendingDataArray.begin(); iter != PendingDataArray.end(); ++iter)
	{
		if(!loadModel(*iter))
		{
			WorldPacket p(CMSG_REQUEST_GEOMETRY);
			p << (*iter).ModelName;
			ASIO::getInstance().sendPacket(p);
			while(!loadModel(*iter)){
				PRINT_FN("Waiting for file .. ");
			}
		}
		else
		{
			vector<FileData>::iterator iter2;
			for(iter2 = ModelDataArray.begin(); iter2 != ModelDataArray.end(); ++iter2)
			{
				if((*iter2).ItemName.compare((*iter).ItemName) == 0)
				{
					ModelDataArray.erase(iter2);
					ModelDataArray.push_back(*iter);
					PendingDataArray.erase(iter);
					_updated = true;
					break;
				}
			}
			if(_updated) break;
			
		}
	}
	return true;
}

void Scene::addPatch(vector<FileData>& data){
	/*for(vector<FileData>::iterator iter = data.begin(); iter != data.end(); iter++)
	{
		for(unordered_map<string,Object3D*>::iterator iter2 = GeometryArray.begin(); iter2 != GeometryArray.end(); iter2++)
			if((iter2->second)->getName().compare((*iter).ModelName) == 0)
			{
				boost::mutex::scoped_lock l(_mutex);
				PendingDataArray.push_back(*iter);
				(iter2->second)->scheduleDeletion();
				GeometryArray.erase(iter2);
				break;
			}
	}*/
}


void Scene::setInitialData(){
	for(vector<FileData>::iterator it = ModelDataArray.begin(); it != ModelDataArray.end();){
		//vegetation is loaded elsewhere
		if((*it).type == VEGETATION){
			VegetationDataArray.push_back(*it);
			it = ModelDataArray.erase(it);
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
		ERROR_FN("SceneManager: Error loading model [ %s ]",  data.ModelName.c_str());
		return false;
	}
	SceneGraphNode* meshNode = _sceneGraph->getRoot()->addNode(thisObj);

	meshNode->getTransform()->scale(data.scale);
	meshNode->getTransform()->rotateEuler(data.orientation);
	meshNode->getTransform()->translate(data.position);
	
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
		
			thisObj =CreateResource<Text3D>(item);
			dynamic_cast<Text3D*>(thisObj)->getWidth() = data.data;
			dynamic_cast<Text3D*>(thisObj)->getText() = data.data2;
	}else{
		ERROR_FN("SCENEMANAGER: Error adding unsupported geometry to scene: [ %s ]",data.ModelName.c_str());
		return false;
	}
	Material* tempMaterial = XML::loadMaterial(data.ItemName+"_material");
	if(!tempMaterial){
		ResourceDescriptor materialDescriptor(data.ItemName+"_material");
		tempMaterial = CreateResource<Material>(materialDescriptor);
		tempMaterial->setDiffuse(data.color);
		tempMaterial->setAmbient(data.color);
	}
	
	thisObj->setMaterial(tempMaterial);
	SceneGraphNode* thisObjSGN = _sceneGraph->getRoot()->addNode(thisObj);
	thisObjSGN->getTransform()->scale(data.scale);
	thisObjSGN->getTransform()->rotateEuler(data.orientation);
	thisObjSGN->getTransform()->translate(data.position);
	return true;
}

void Scene::addLight(Light* const lightItem){
	LightManager::getInstance().addLight(lightItem);
}

Light* Scene::addDefaultLight(){
	stringstream ss; ss << LightManager::getInstance().getLights().size();
	ResourceDescriptor defaultLight("Default omni light "+ss.str());
	defaultLight.setId(0); //descriptor ID is not the same as light ID. This is the light's slot!!
	defaultLight.setResourceLocation("root");
	Light* l = CreateResource<Light>(defaultLight);
	l->setLightType(LIGHT_DIRECTIONAL);
	_sceneGraph->getRoot()->addNode(l);
	addLight(l);
	vec4<F32> ambientColor(0.1f, 0.1f, 0.1f, 1.0f);
	LightManager::getInstance().setAmbientLight(ambientColor);
	return l;	
}

SceneGraphNode* Scene::addGeometry(Object3D* const object){
	return _sceneGraph->getRoot()->addNode(object);
}

bool Scene::removeGeometry(SceneNode* node){
	if(!node) {
		ERROR_FN("Trying to delete NULL scene node!");
		return false;
	}
	SceneGraphNode* _graphNode = _sceneGraph->findNode(node->getName());

	SAFE_DELETE_CHECK(_graphNode);

}

bool Scene::load(const std::string& name){
	SceneGraphNode* root = _sceneGraph->getRoot();
	if(TerrainInfoArray.empty()) return true;
	for(U8 i = 0; i < TerrainInfoArray.size(); i++){
		ResourceDescriptor terrain(TerrainInfoArray[i]->getVariable("terrainName"));
		Terrain* temp = CreateResource<Terrain>(terrain);
		SceneGraphNode* terrainTemp = root->addNode(temp);
		terrainTemp->useDefaultTransform(false);
		terrainTemp->setTransform(NULL);
		terrainTemp->setActive(TerrainInfoArray[i]->getActive());
		temp->initializeVegetation(TerrainInfoArray[i]);
	}
	return true;
}

bool Scene::unload(){
	clearEvents();
	//_inputEvent.get()->stopEvent();
	//_inputEvent.reset();
	if(_aiEvent.get()){
		_aiEvent.get()->stopEvent();
		_aiEvent.reset();
	}
	_inputManager.terminate();
	_inputManager.DestroyInstance();

	SAFE_DELETE(_lightTexture);
	SAFE_DELETE(_deferredBuffer);

	if(_deferredShader != NULL){
		RemoveResource(_deferredShader);
	}
	AIManager::getInstance().Destroy();
	AIManager::getInstance().DestroyInstance();
	clearObjects();
	clearLights();
	return true;
}

void Scene::clearObjects(){
	for(U8 i = 0; i < TerrainInfoArray.size(); i++){
		RemoveResource(TerrainInfoArray[i]);
	}
	TerrainInfoArray.clear();
	ModelDataArray.clear();
	VegetationDataArray.clear();
	PendingDataArray.clear();
	assert(_sceneGraph);

	SAFE_DELETE(_sceneGraph);
}

void Scene::clearLights(){
	LightManager::getInstance().clear();
}

void Scene::clearEvents()
{
	PRINT_FN("Stopping all events ...");
	_events.clear();
}

void Scene::onMouseMove(const OIS::MouseEvent& key){
	GUI::getInstance().checkItem(key.state.X.abs,key.state.Y.abs);
}

void Scene::processInput(){
	_inputManager.tick();
	//AIManager::getInstance().tick();
}

bool Scene::loadEvents(bool continueOnErrors){
	//Running the input manager through a separate thread degrades performance. Enable at your own risk - Ionut
	//_inputEvent.reset(New Event(1,true,false,boost::bind(&InputManagerInterface::tick, boost::ref(InputManagerInterface::getInstance()))));
	_aiEvent.reset(New Event(3,true,false,boost::bind(&AIManager::tick,boost::ref(AIManager::getInstance()))));
	return true;
}

void Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button){
	switch(button){
		case OIS::MB_Left:{
			GUI::getInstance().clickCheck();
		}break;

		case OIS::MB_Right:{
		}break;

		case OIS::MB_Middle:{
		}break;

		case OIS::MB_Button3:
		case OIS::MB_Button4:
		case OIS::MB_Button5:
		case OIS::MB_Button6:
		case OIS::MB_Button7:
		default:
			break;
	}
}

void Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button){

	switch(button){
		case OIS::MB_Left:{
			GUI::getInstance().clickReleaseCheck();
		}break;

		case OIS::MB_Right:	{
		}break;

		case OIS::MB_Middle:{
		}break;

		case OIS::MB_Button3:
		case OIS::MB_Button4:
		case OIS::MB_Button5:
		case OIS::MB_Button6:
		case OIS::MB_Button7:
		default:
			break;
	}
}

static F32 speedFactor = 0.25f;
void Scene::onKeyDown(const OIS::KeyEvent& key){

	switch(key.key){
		case OIS::KC_LEFT : 
			Application::getInstance().angleLR = -(speedFactor/5);
			break;
		case OIS::KC_RIGHT : 
			Application::getInstance().angleLR = speedFactor/5;
			break;
		case OIS::KC_UP : 
			Application::getInstance().angleUD = -(speedFactor/5);
			break;
		case OIS::KC_DOWN : 
			Application::getInstance().angleUD = speedFactor/5;
			break;
		case OIS::KC_END:
			SceneManager::getInstance().deleteSelection();
			break;
		case OIS::KC_R:
			Guardian::getInstance().ReloadEngine();
			break;
		case OIS::KC_B:{
			PRINT_FN("Toggling Bounding Boxes");
			SceneManager::getInstance().toggleBoundingBoxes();
					   }break;
		case OIS::KC_ADD:
			if (speedFactor < 10)  speedFactor += 0.1f;
			break;
		case OIS::KC_SUBTRACT:
			if (speedFactor > 0.1f)   speedFactor -= 0.1f;
			break;
		case OIS::KC_F10:
			Application::getInstance().togglePreviewDepthMaps();
			break;
		case OIS::KC_F12:
			GFX_DEVICE.Screenshot("screenshot_",vec4<F32>(0,0,Application::getInstance().getWindowDimensions().x,Application::getInstance().getWindowDimensions().y));
			break;
		default:
			break;
	}
}

void Scene::onKeyUp(const OIS::KeyEvent& key){

	switch( key.key ){
		case OIS::KC_LEFT:
		case OIS::KC_RIGHT:
			Application::getInstance().angleLR = 0.0f;
			break;
		case OIS::KC_UP:
		case OIS::KC_DOWN:
			Application::getInstance().angleUD = 0.0f;
			break;
		case OIS::KC_F:
			bool pdc = _paramHandler.getParam<bool>("enableDepthOfField");
			_paramHandler.setParam("enableDepthOfField", !pdc); 
			break;
	}
}

void Scene::OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis){

	if(axis == 1){
		if(key.state.mAxes[axis].abs > 0)
			Application::getInstance().angleLR = speedFactor/5;
		else if(key.state.mAxes[axis].abs < 0)
			Application::getInstance().angleLR = -(speedFactor/5);
		else
			Application::getInstance().angleLR = 0;

	}else if(axis == 0){

		if(key.state.mAxes[axis].abs > 0)
			Application::getInstance().angleUD = speedFactor/5;
		else if(key.state.mAxes[axis].abs < 0)
			Application::getInstance().angleUD = -(speedFactor/5);
		else
			Application::getInstance().angleUD = 0;
	}
}

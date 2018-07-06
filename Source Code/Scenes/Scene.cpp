#include "Scene.h"
#include "Managers/SceneManager.h" //Object selection
#include "Managers/AIManager.h"
#include "PhysX/PhysX.h"
#include "ASIO.h"
#include "Terrain/Terrain.h"
#include "Utility/Headers/Guardian.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "GUI/GUI.h"
#include "Utility/Headers/XMLParser.h"
using namespace std;


bool Scene::clean(){ //Called when application is idle
	if(_sceneGraph){
		if(_sceneGraph->getRoot()->getChildren().empty()) return false;
	}

	bool _updated = false;
	if(!PendingDataArray.empty())
	for(vector<FileData>::iterator iter = PendingDataArray.begin(); iter != PendingDataArray.end(); iter++)
	{
		if(!loadModel(*iter))
		{
			WorldPacket p(CMSG_REQUEST_GEOMETRY);
			p << (*iter).ModelName;
			ASIO::getInstance().sendPacket(p);
			while(!loadModel(*iter)){
				Console::getInstance().printfn("Waiting for file .. ");
			}
		}
		else
		{
			vector<FileData>::iterator iter2;
			for(iter2 = ModelDataArray.begin(); iter2 != ModelDataArray.end(); iter2++)
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

bool Scene::loadModel(const FileData& data)
{
	if(data.type == PRIMITIVE)	return loadGeometry(data);

	ResourceDescriptor model(data.ItemName);
	model.setResourceLocation(data.ModelName);
	model.setFlag(true);
	Mesh *thisObj = _resManager.loadResource<Mesh>(model);
	if (!thisObj){
		Console::getInstance().errorfn("SceneManager: Error loading model [ %s ]",  data.ModelName.c_str());
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
			thisObj = _resManager.loadResource<Box3D>(item);
			dynamic_cast<Box3D*>(thisObj)->getSize() = data.data;

	} else if(data.ModelName.compare("Sphere3D") == 0) {
			thisObj = _resManager.loadResource<Sphere3D>(item);
			dynamic_cast<Sphere3D*>(thisObj)->getSize() = data.data;

	} else if(data.ModelName.compare("Quad3D") == 0)	{
			vec3 scale = data.scale;
			vec3 position = data.position;
			thisObj = _resManager.loadResource<Quad3D>(item);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::TOP_LEFT)     = vec3(0,1,0);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::TOP_RIGHT)    = vec3(1,1,0);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::BOTTOM_LEFT)  = vec3(0,0,0);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(1,0,0);
	} else if(data.ModelName.compare("Text3D") == 0) {
		
			thisObj =_resManager.loadResource<Text3D>(item);
			dynamic_cast<Text3D*>(thisObj)->getWidth() = data.data;
			dynamic_cast<Text3D*>(thisObj)->getText() = data.data2;
	}else{
		Console::getInstance().errorfn("SCENEMANAGER: Error adding unsupported geometry to scene: [ %s ]",data.ModelName.c_str());
		return false;
	}
	Material* tempMaterial = XML::loadMaterial(data.ItemName+"_material");
	if(!tempMaterial){
		ResourceDescriptor materialDescriptor(data.ItemName+"_material");
		tempMaterial = ResourceManager::getInstance().loadResource<Material>(materialDescriptor);
	}
	
	thisObj->setMaterial(tempMaterial);
	thisObj->getMaterial()->setDiffuse(data.color);
	thisObj->getMaterial()->setAmbient(data.color);
	SceneGraphNode* thisObjSGN = _sceneGraph->getRoot()->addNode(thisObj);
	thisObjSGN->getTransform()->scale(data.scale);
	thisObjSGN->getTransform()->rotateEuler(data.orientation);
	thisObjSGN->getTransform()->translate(data.position);

	return true;
}

bool Scene::addDefaultLight(){
	
	F32 oldSize = _lights.size();
	stringstream ss; ss << oldSize;
	ResourceDescriptor defaultLight("Default omni light "+ss.str());
	defaultLight.setId(0);
	defaultLight.setResourceLocation("root");
	Light* l = _resManager.loadResource<Light>(defaultLight);
	_sceneGraph->getRoot()->addNode(l);
	_lights.push_back(l);
	if(_lights.size() > oldSize) return true;	
	else return false;
}

SceneGraphNode* Scene::addGeometry(Object3D* const object){
	return _sceneGraph->getRoot()->addNode(object);
}

bool Scene::removeGeometry(SceneNode* node){
	if(!node) {
		Console::getInstance().errorfn("Trying to delete NULL scene node!");
		return false;
	}
	SceneGraphNode* _graphNode = node->getSceneGraphNode();
	if(_graphNode){
		delete _graphNode;
		_graphNode = NULL;
		return true;
	}
	return false;
}

bool Scene::load(const std::string& name){
	SceneGraphNode* root = _sceneGraph->getRoot();
	if(TerrainInfoArray.empty()) return true;
	for(U8 i = 0; i < TerrainInfoArray.size(); i++){
		ResourceDescriptor terrain(TerrainInfoArray[i]->getVariable("terrainName"));
		Terrain* temp = _resManager.loadResource<Terrain>(terrain);
		SceneGraphNode* terrainTemp = _sceneGraph->getRoot()->addNode(temp);
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
	_aiEvent.get()->stopEvent();
	_aiEvent.reset();
	_inputManager.terminate();
	_inputManager.DestroyInstance();
	if(_lightTexture != NULL){
		delete _lightTexture;
		_lightTexture = NULL;
	}
	if(_deferredBuffer != NULL){
		delete _deferredBuffer;
		_deferredBuffer = NULL;
	}
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
	delete _sceneGraph;
	_sceneGraph = NULL;
}

void Scene::clearLights(){
	_lights.empty();
}

void Scene::clearEvents()
{
	Console::getInstance().printfn("Stopping all events ...");
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
			//SceneManager::getInstance().findSelection(key.state.X.rel,key.state.Y.rel);
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
			Application::getInstance().angleLR = -(speedFactor/8);
			break;
		case OIS::KC_RIGHT : 
			Application::getInstance().angleLR = speedFactor/8;
			break;
		case OIS::KC_UP : 
			Application::getInstance().angleUD = -(speedFactor/8);
			break;
		case OIS::KC_DOWN : 
			Application::getInstance().angleUD = speedFactor/8;
			break;
		case OIS::KC_END:
			SceneManager::getInstance().deleteSelection();
			break;
		case OIS::KC_R:
			Guardian::getInstance().ReloadEngine();
			break;
		case OIS::KC_P:
			Guardian::getInstance().RestartPhysX();
			break;
		case OIS::KC_N:
			_GFX.toggleWireframe();
			break;
		case OIS::KC_B:
			SceneManager::getInstance().toggleBoundingBoxes();
			break;
		case OIS::KC_ADD:
			if (speedFactor < 10)  speedFactor += 0.1f;
			break;
			case OIS::KC_SUBTRACT:
			if (speedFactor > 0.1f)   speedFactor -= 0.1f;
			break;
		//1+2+3+4 = cream diversi actori prin scena
		case OIS::KC_1:
			PhysX::getInstance().CreateStack(20);
			break;
		case OIS::KC_2:
			PhysX::getInstance().CreateTower(15);
			break;
		case OIS::KC_3:
			PhysX::getInstance().CreateSphere(8);
			break;
		case OIS::KC_4:
			PhysX::getInstance().CreateCube(20);
			break;
		case OIS::KC_5:
			PhysX::getInstance().setDebugRender(!PhysX::getInstance().getDebugRender());
			break;
		case OIS::KC_6:
			PhysX::getInstance().ApplyForceToActor(PhysX::getInstance().GetSelectedActor(), NxVec3(+1,0,0), 3000); 
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
			Application::getInstance().angleLR = speedFactor/8;
		else if(key.state.mAxes[axis].abs < 0)
			Application::getInstance().angleLR = -(speedFactor/8);
		else
			Application::getInstance().angleLR = 0;

	}else if(axis == 0){

		if(key.state.mAxes[axis].abs > 0)
			Application::getInstance().angleUD = speedFactor/8;
		else if(key.state.mAxes[axis].abs < 0)
			Application::getInstance().angleUD = -(speedFactor/8);
		else
			Application::getInstance().angleUD = 0;
	}
}

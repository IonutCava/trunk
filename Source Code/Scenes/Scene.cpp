#include "Scene.h"
#include "Managers/ResourceManager.h"
#include "Managers/SceneManager.h" //Object selection
#include "PhysX/PhysX.h"
#include "ASIO.h"
#include "Utility/Headers/Guardian.h"
#include "Geometry/Predefined/Box3D.h"
#include "Geometry/Predefined/Quad3D.h"
#include "Geometry/Predefined/Sphere3D.h"
#include "Geometry/Predefined/Text3D.h"
#include "GUI/GUI.h"

using namespace std;

Scene::~Scene()
{
	unload();
}

bool Scene::clean() //Called when application is idle
{
	if(GeometryArray.empty()) return false;

	bool _updated = false;
	if(!PendingDataArray.empty())
	for(vector<FileData>::iterator iter = PendingDataArray.begin(); iter != PendingDataArray.end(); iter++)
	{
		if(!loadModel(*iter))
		{
			WorldPacket p(CMSG_REQUEST_GEOMETRY);
			p << (*iter).ModelName;
			ASIO::getInstance().sendPacket(p);
			while(!loadModel(*iter))
			{
				Con::getInstance().printfn("Waiting for file .. ");

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

void Scene::addPatch(vector<FileData>& data)
{
	for(vector<FileData>::iterator iter = data.begin(); iter != data.end(); iter++)
	{
		for(tr1::unordered_map<string,Object3D*>::iterator iter2 = GeometryArray.begin(); iter2 != GeometryArray.end(); iter2++)
			if((iter2->second)->getName().compare((*iter).ModelName) == 0)
			{
				boost::mutex::scoped_lock l(_mutex);
				PendingDataArray.push_back(*iter);
				(iter2->second)->scheduleDeletion();
				GeometryArray.erase(iter2);
				break;
			}
	}
}


void Scene::setInitialData()
{
	for(vector<FileData>::iterator it = ModelDataArray.begin(); it != ModelDataArray.end();)
	{
		//vegetation is loaded elsewhere
		if((*it).type == VEGETATION)
		{
			VegetationDataArray.push_back(*it);
			it = ModelDataArray.erase(it);
		}
		else
		{
			loadModel(*it);
			++it;
		}

	}
}	

bool Scene::loadModel(const FileData& data)
{
	if(data.type == PRIMITIVE) 
		return loadGeometry(data);

	Mesh *thisObj = ResourceManager::getInstance().LoadResource<Mesh>(data.ModelName);
	if (!thisObj)
	{
		Con::getInstance().errorfn("SceneManager: Error loading model [ %s ]",  data.ModelName.c_str());
		return false;
	}
	thisObj->setName(data.ModelName);	
	
	
	thisObj->getTransform()->scale(data.scale);
	thisObj->getTransform()->rotateEuler(data.orientation);
	thisObj->getTransform()->translate(data.position);

	pair<tr1::unordered_map<string,Object3D*>::iterator,bool> _result;
	_result = GeometryArray.insert(pair<string,Object3D*>(data.ItemName,thisObj));
	if(!_result.second) (_result.first)->second = thisObj;

	return true;
}

bool Scene::loadGeometry(const FileData& data)
{
	Object3D* thisObj;
	if(data.ModelName.compare("Box3D") == 0) {
			thisObj = ResourceManager::getInstance().LoadResource<Box3D>(data.ModelName+": "+data.ItemName);
			dynamic_cast<Box3D*>(thisObj)->getSize() = data.data;

	} else if(data.ModelName.compare("Sphere3D") == 0) {
			thisObj = ResourceManager::getInstance().LoadResource<Sphere3D>(data.ModelName+": "+data.ItemName);
			dynamic_cast<Sphere3D*>(thisObj)->getSize() = data.data;

	} else if(data.ModelName.compare("Quad3D") == 0)	{
			vec3 scale = data.scale;
			vec3 position = data.position;
			thisObj = ResourceManager::getInstance().LoadResource<Quad3D>(data.ModelName+": "+data.ItemName);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::TOP_LEFT)     = vec3(scale.x/2-position.x,scale.y/2+position.y, scale.z/2 + position.z);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::TOP_RIGHT)    = vec3(scale.x/2+position.x,scale.y/2+position.y, scale.z/2 + position.z);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::BOTTOM_LEFT)  = vec3(scale.x/2-position.x,scale.y/2-position.y, scale.z/2 + position.z);
			dynamic_cast<Quad3D*>(thisObj)->getCorner(Quad3D::BOTTOM_RIGHT) = vec3(scale.x/2+position.x,scale.y/2-position.y, scale.z/2 + position.z);

	} else if(data.ModelName.compare("Text3D") == 0) {
			thisObj = ResourceManager::getInstance().LoadResource<Text3D>(data.ModelName+": "+data.ItemName);
			dynamic_cast<Text3D*>(thisObj)->getWidth() = data.data;
			dynamic_cast<Text3D*>(thisObj)->getText() = data.data2;
	}
	else
	{
		Con::getInstance().errorfn("SCENEMANAGER: Error adding unsupported geometry to scene: [ %s ]",data.ModelName.c_str());
		return false;
	}
	thisObj->getMaterial().diffuse = data.color;
	thisObj->getTransform()->scale(data.scale);
	thisObj->getTransform()->rotateEuler(data.orientation);
	thisObj->getTransform()->translate(data.position);

	pair<tr1::unordered_map<string,Object3D*>::iterator,bool> _result;
	_result = GeometryArray.insert(pair<string,Object3D*>(thisObj->getName(),thisObj));
	if(!_result.second) (_result.first)->second = thisObj;
	return true;
}

bool Scene::addDefaultLight(){
	F32 oldSize = _lights.size();

	Light_ptr l(new Light(oldSize));
	l->update();
	_lights.push_back(l);
	if(_lights.size() > oldSize) return true;	
	else return false;

}

void Scene::addGeometry(Object3D* const object){
	GeometryArray.insert(pair<string,Object3D*>(object->getName(),object));
}

bool Scene::removeGeometry(const std::string& name)
{
	if(GeometryArray.find(name) != GeometryArray.end())
	{
		ResourceManager::getInstance().remove(GeometryArray[name]);
		GeometryArray.erase(name);
		return true;
	}
	return false;
}

bool Scene::unload()
{
	clearEvents();
	_inputManager.terminate();
	_inputManager.DestroyInstance();
	if(_lightTexture){
		delete _lightTexture;
		_lightTexture = NULL;
	}
	if(_deferredBuffer){
		delete _deferredBuffer;
		_deferredBuffer = NULL;
	}
	ResourceManager::getInstance().remove(_deferredShader);
	clearObjects();
	clearLights();
	
	return true;
}

void Scene::clearObjects()
{
	for(GeometryIterator = GeometryArray.begin(); GeometryIterator != GeometryArray.end(); GeometryIterator++){
		ResourceManager::getInstance().remove(GeometryIterator->second);
	}
	GeometryArray.clear(); 
	ModelDataArray.clear();
	VegetationDataArray.clear();
	PendingDataArray.clear();
	TerrainInfoArray.clear();
}

void Scene::clearLights()
{
	_lights.empty();
}

void Scene::clearEvents()
{
	Con::getInstance().printfn("Stopping all events ...");
	_events.clear();
}

void Scene::onMouseMove(const OIS::MouseEvent& key)
{
	GUI::getInstance().checkItem(key.state.X.abs,key.state.Y.abs);
}

void Scene::onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button)
{
	switch(button)
	{
		case OIS::MB_Left:
		{
			GUI::getInstance().clickCheck();
		}break;

		case OIS::MB_Right:
		{
		}break;

		case OIS::MB_Middle:
		{
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

void Scene::onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button)
{
	switch(button)
	{
		case OIS::MB_Left:
		{
			//SceneManager::getInstance().findSelection(key.state.X.rel,key.state.Y.rel);
			GUI::getInstance().clickReleaseCheck();
		}break;

		case OIS::MB_Right:
		{
		}break;

		case OIS::MB_Middle:
		{
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
void Scene::onKeyDown(const OIS::KeyEvent& key)
{
	switch(key.key)
	{
		case OIS::KC_LEFT : 
			Engine::getInstance().angleLR = -(speedFactor/10);
			break;
		case OIS::KC_RIGHT : 
			Engine::getInstance().angleLR = speedFactor/10;
			break;
		case OIS::KC_UP : 
			Engine::getInstance().angleUD = -(speedFactor/10);
			break;
		case OIS::KC_DOWN : 
			Engine::getInstance().angleUD = speedFactor/10;
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
			GFXDevice::getInstance().toggleWireframe();
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
		case OIS::KC_7:
			Guardian::getInstance().TerminateApplication();
			break;
		default:
			break;
	}
}

void Scene::onKeyUp(const OIS::KeyEvent& key)
{
	switch( key.key )
	{
		case OIS::KC_LEFT:
		case OIS::KC_RIGHT:
			Engine::getInstance().angleLR = 0.0f;
			break;
		case OIS::KC_UP:
		case OIS::KC_DOWN:
			Engine::getInstance().angleUD = 0.0f;
			break;
	}
}

void Scene::OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis)
{
	if(axis == 1)
	{
		if(key.state.mAxes[axis].abs > 0)
			Engine::getInstance().angleLR = speedFactor/10;
		else if(key.state.mAxes[axis].abs < 0)
			Engine::getInstance().angleLR = -(speedFactor/10);
		else
			Engine::getInstance().angleLR = 0;
	}
	else if(axis == 0)
	{
		if(key.state.mAxes[axis].abs > 0)
			Engine::getInstance().angleUD = speedFactor/10;
		else if(key.state.mAxes[axis].abs < 0)
			Engine::getInstance().angleUD = -(speedFactor/10);
		else
			Engine::getInstance().angleUD = 0;
	}
}

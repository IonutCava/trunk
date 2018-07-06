#include "Scene.h"
#include "Managers/ResourceManager.h"
#include "PhysX/PhysX.h"
#include "ASIO.h"

void Scene::clean() //Called when application is idle
{
	if(ModelArray.empty()) return;
	for(vector<DVDFile*>::iterator _iter = ModelArray.begin(); _iter != ModelArray.end(); _iter++)
	{
		if((*_iter)->clean())
		{
			delete *_iter;
		    *_iter = NULL;
			ModelArray.erase(_iter);
			break;
		}
	}
	bool _updated = false;

	for(vector<FileData>::iterator _iter = PendingDataArray.begin(); _iter != PendingDataArray.end(); _iter++)
	{
		if(!loadModel(*_iter))
		{
			WorldPacket p(CMSG_REQUEST_GEOMETRY);
			p << (*_iter).ModelName;
			ASIO::getInstance().sendPacket(p);
		}
		else
		{
			for(vector<FileData>::iterator _iter2 = ModelDataArray.begin(); _iter2 != ModelDataArray.end(); _iter2++)
			{
				if((*_iter2).ModelName.compare((*_iter).ModelName) == 0)
				{
					ModelDataArray.erase(_iter2);
					ModelDataArray.push_back(*_iter);
					PendingDataArray.erase(_iter);
					_updated = true;
					break;
				}
			}
			if(_updated) break;
			
		}
	}
}

void Scene::addPatch(vector<FileData>& data)
{
	for(vector<FileData>::iterator _iter = data.begin(); _iter != data.end(); _iter++)
		for(vector<DVDFile*>::iterator _iter2 = ModelArray.begin(); _iter2 != ModelArray.end(); _iter2++)
			if((*_iter2)->getName().compare((*_iter).ModelName) == 0)
			{
				cout << "A" << endl;
				(*_iter2)->scheduleDeletion(); //ToDo: Fix OpenGL context switch between threads; -Ionut
			
				cout << "SCENE: new pending geometry added in queue [ " << (*_iter).ModelName << endl;
				//ToDo: MUTEX!!!!!!!!!!!!!!!! -Ionut
				boost::mutex::scoped_lock l(_mutex);
				PendingDataArray.push_back(*_iter);
			}
}


void Scene::setInitialData()
{
	if (ModelDataArray.empty()) return;

	for(U32 index = 0; index  < ModelDataArray.size(); index++)
	{
		//vegetation is loaded elsewhere
		if(ModelDataArray[index].type == VEGETATION)
		{
			VegetationDataArray.push_back(ModelDataArray[index]);
			ModelDataArray.erase(ModelDataArray.begin() + index);
			continue;
		}
		loadModel(ModelDataArray[index]);
	}
}	

bool Scene::loadModel(FileData& data)
{
	DVDFile *thisObj = ResourceManager::getInstance().LoadResource<DVDFile>(data.ModelName);
	if (!thisObj)
	{
		cout << "SceneManager: Error loading model " << &data.ModelName << endl;
		return false;
	}
		
	thisObj->getOrientation() = data.orientation;

	thisObj->setScale(data.scale);
	thisObj->setPosition(data.position);
		
	thisObj->setShader(ResourceManager::getInstance().LoadResource<Shader>("OBJ"));
	ModelArray.push_back(thisObj);
	return true;
}
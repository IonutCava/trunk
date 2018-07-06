#include "Scene.h"
#include "Managers/ResourceManager.h"
#include "PhysX/PhysX.h"

void Scene::clean() //Called when application is idle
{
	//for(vector<DVDFile*>::iterator _iter = ModelArray.begin(); _iter != ModelArray.end(); _iter++)
	//	(*_iter)->clean();
}

void Scene::addPatch(vector<FileData>& data)
{
	for(vector<FileData>::iterator _iter = data.begin(); _iter != data.end(); _iter++)
		for(vector<DVDFile*>::iterator _iter2 = ModelArray.begin(); _iter2 != ModelArray.end(); _iter2++)
			if((*_iter2)->getName().compare((*_iter).ModelName) == 0)
			{
				(*_iter2)->scheduleDeletion(); //ToDo: Fix OpenGL context switch between threads; -Ionut
				ModelArray.erase(_iter2);
				loadModel(*_iter);
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

void Scene::loadModel(FileData& data)
{
	DVDFile *thisObj = ResourceManager::getInstance().LoadResource<DVDFile>(data.ModelName);
	if (!thisObj)
	{
		cout << "SceneManager: Error loading model " << &data.ModelName << endl;
		return;
	}
		
	thisObj->getOrientation() = data.orientation;

	thisObj->setScale(data.scale);
	thisObj->setPosition(data.position);
		
	thisObj->setShader(ResourceManager::getInstance().LoadResource<Shader>("OBJ"));
	ModelArray.push_back(thisObj);
}
#include "Scene.h"
#include "Managers/ResourceManager.h"
#include "PhysX/PhysX.h"

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

		DVDFile *thisObj = ResourceManager::getInstance().LoadResource<DVDFile>(ModelDataArray[index].ModelName);
		if (!thisObj)
		{
			cout << "SceneManager: Error loading model " << &ModelDataArray[index].ModelName << endl;
			return;
		}

		
		thisObj->getOrientation() = ModelDataArray[index].orientation;

		thisObj->setScale(ModelDataArray[index].scale);
		thisObj->setPosition(ModelDataArray[index].position);
		
		thisObj->setShader(ResourceManager::getInstance().LoadResource<Shader>("OBJ"));
		ModelArray.push_back(thisObj);
	}
}	

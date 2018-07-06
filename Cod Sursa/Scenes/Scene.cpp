#include "Scene.h"
#include "Importer/objimporter.h"
#include "PhysX/PhysX.h"

ImportedModel* Scene::findObject(string& name)
{
	for(ModelIterator = ModelArray.begin();  ModelIterator != ModelArray.end();  ModelIterator++)
    {
		if((*ModelIterator)->ModelName.compare("assets/modele/"+name) == 0)
		{
			return (*ModelIterator);
		}
	}

	return NULL;
}

void Scene::setInitialData(const vector<FileData>& models)
{
	for(U32 index = 0; index  < models.size(); index++)
	{
		//vegetation is loaded elsewhere
		if(models[index].Vegetation) return;

		ImportedModel *thisObj = ReadOBJ(models[index].ModelName.c_str(),models[index].position,models[index].ScaleFactor,models[index].rotation,false);
		//glmSetNormalMap(thisObj,DA[index].NormalMap);
		if (!thisObj)
		{
			cout << "Error loading model " << &models[index].ModelName << endl;
			return;
		}
		//Guardian::getInstance().AddMesh(thisObj);
		PhysX::getInstance().AddShape(thisObj,false);
		ModelArray.push_back(thisObj);
		glmList(thisObj, GLM_SMOOTH | GLM_TEXTURE | GLM_MATERIAL | GLM_2_SIDED );
	}
}	

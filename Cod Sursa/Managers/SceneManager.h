#include "resource.h"
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Manager.h"
#include "Scenes/Scene.h"
#include "Scenes/MainScene.h"
#include "Scenes/CubeScene.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(SceneManager,Manager)

public:
	Scene& getActiveScene() {return *_scene;}
	void setActiveScene(Scene* scene) {_scene = scene;}

	/*Base Scene Operations*/
	void render() {_scene->render();}
	void preRender() {_scene->preRender();}
	bool load(const string& name) {_scene->setInitialData(); return _scene->load(name);}
	bool unload() {return _scene->unload();}
	void processInput(){_scene->processInput();}
	void processEvents(F32 time){_scene->processEvents(time);}
	/*Base Scene Operations*/

	inline vector<ImportedModel*>& getModelArray(){return _scene->getModelArray();}

	inline vector<FileData>& getModelDataArray() {return _scene->getModelDataArray();}
	inline vector<FileData>& getVegetationDataArray() {return _scene->getVegetationDataArray();}

	int getNumberOfObjects(){return _scene->getNumberOfObjects();}
    int getNumberOfTerrains(){return _scene->getNumberOfTerrains();}
	TerrainManager* getTerrainManager() {return _scene->getTerrainManager();}
   
	Scene* findScene(const std::string& name);

	void addModel(FileData& model){_scene->addModel(model);}
	void addTerrain(const TerrainInfo& ter) {_scene->addTerrain(ter);}
	void toggleBoundingBoxes();

private:
	SceneManager()
	{
		_scenes.insert(make_pair("MainScene", New MainScene()));
		_scenes.insert(make_pair("CubeScene", New CubeScene()));
	}
	Scene* _scene;
	map<string, Scene*> _scenes;
	map<string, Scene*>::iterator _sceneIter;

SINGLETON_END()

#endif


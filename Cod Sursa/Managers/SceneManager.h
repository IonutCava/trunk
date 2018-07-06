#include "resource.h"
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Manager.h"
#include "Scenes/Scene.h"
#include "SceneList.h"
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

	inline vector<DVDFile* >& getModelArray(){return _scene->getModelArray();}
	inline vector<FileData>& getModelDataArray() {return _scene->getModelDataArray();}
	inline vector<FileData>& getVegetationDataArray() {return _scene->getVegetationDataArray();}

	int getNumberOfObjects(){return _scene->getNumberOfObjects();}
    int getNumberOfTerrains(){return _scene->getNumberOfTerrains();}
	TerrainManager* getTerrainManager() {return _scene->getTerrainManager();}
   
	Scene* findScene(const std::string& name);

	void addModel(FileData& model){_scene->addModel(model);}
	void addTerrain(const TerrainInfo& ter) {_scene->addTerrain(ter);}
	void toggleBoundingBoxes();
	void addPatch(vector<FileData>& data){_scene->addPatch(data);}
	
	void findSelection(int x, int y);
	void deleteSelection();

	void clean(){_scene->clean();}
private:

	SceneManager()
	{
		_scenes.insert(make_pair("MainScene", New MainScene()));
		_scenes.insert(make_pair("CubeScene", New CubeScene()));
		_scenes.insert(make_pair("NetworkScene", New NetworkScene()));
		_currentSelection = NULL;
	}
	Scene* _scene;
	map<string, Scene*> _scenes;
	map<string, Scene*>::iterator _sceneIter;
    DVDFile* _currentSelection;

SINGLETON_END()

#endif


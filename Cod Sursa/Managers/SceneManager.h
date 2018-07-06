#include "resource.h"
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Manager.h"
#include "Scenes/Scene.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(SceneManager,Manager)

public:
	Scene* getActiveScene() {return _scene;}
	void setActiveScene(Scene* scene) {if(_scene) delete _scene; _scene = scene;}

	/*Base Scene Operations*/
	void render() {_scene->render();}
	void preRender() {_scene->preRender();}
	bool load(const std::string& name) {_scene->setInitialData(); return _scene->load(name);}
	bool unload() {return _scene->unload();}
	void processInput(){_scene->processInput();}
	void processEvents(F32 time){_scene->processEvents(time);}
	/*Base Scene Operations*/

	inline std::tr1::unordered_map<std::string, Mesh* >& getModelArray(){return _scene->getModelArray();}
	inline std::tr1::unordered_map<std::string, Object3D* >& getGeometryArray(){return _scene->getGeometryArray();}
	inline std::vector<FileData>& getModelDataArray() {return _scene->getModelDataArray();}
	inline std::vector<FileData>& getVegetationDataArray() {return _scene->getVegetationDataArray();}

	int getNumberOfObjects(){return _scene->getNumberOfObjects();}
    int getNumberOfTerrains(){return _scene->getNumberOfTerrains();}
	TerrainManager* getTerrainManager() {return _scene->getTerrainManager();}
   
	Scene* findScene(const std::string& name);

	void addModel(FileData& model){_scene->addModel(model);}
	void addTerrain(const TerrainInfo& ter) {_scene->addTerrain(ter);}
	void toggleBoundingBoxes();
	void addPatch(std::vector<FileData>& data){_scene->addPatch(data);}
	
	void findSelection(int x, int y);
	void deleteSelection();

	void clean(){_scene->clean();}

private:

	SceneManager();
	Scene* _scene;
	std::map<std::string, Scene*> _scenes;
	std::map<std::string, Scene*>::iterator _sceneIter;
    Mesh* _currentSelection;

SINGLETON_END()

#endif


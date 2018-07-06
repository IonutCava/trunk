#include "resource.h"
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Manager.h"
#include "Scenes/Scene.h"
#include "Utility/Headers/Singleton.h"

SINGLETON_BEGIN_EXT1(SceneManager,Manager)

public:
	Scene& getActiveScene() {return *_scene;}
	void setActiveScene(Scene* scene) {_scene = scene;}

	/*Base Scene Operations*/
	void render() {_scene->render();}
	void preRender() {_scene->preRender();}
	bool load(const string& name) {return _scene->load(name);}
	bool unload() {return _scene->unload();}
	void processInput(){_scene->processInput();}
	void processEvents(F32 time){_scene->processEvents(time);}
	/*Base Scene Operations*/

	inline vector<ImportedModel*>& getModelArray(){return _scene->getModelArray();}
	int getNumberOfObjects(){return _scene->getNumberOfObjects();}
    int getNumberOfTerrains(){return _scene->getNumberOfTerrains();}
    void setNumberOfObjects(int nr){_scene->setNumberOfObjects(nr);}
    void setNumberOfTerrains(int nr){_scene->setNumberOfTerrains(nr);}
    void incNumberOfObjects(){_scene->incNumberOfObjects();}
    void incNumberOfTerrains(){_scene->incNumberOfTerrains();}
	ImportedModel* findObject(string& name){return _scene->findObject(name);}
	void setInitialData(const vector<FileData>& models){_scene->setInitialData(models);}

	

private:
	SceneManager(){}
	Scene* _scene;

SINGLETON_END()

#endif


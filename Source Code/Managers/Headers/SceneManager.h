/*“Copyright 2009-2012 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "resource.h"
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Scenes/Headers/Scene.h"

DEFINE_SINGLETON_EXT1(SceneManager,Manager)

public:
	Scene* getActiveScene() {return _scene;}
	inline void setActiveScene(Scene* scene) {if(_scene) delete _scene; _scene = scene;}

	/*Base Scene Operations*/
	void render(RENDER_STAGE stage);
	inline void preRender() {_scene->preRender();}
	bool load(const std::string& name);
	inline bool unload() {return _scene->unload();}
	inline void processInput(){_scene->processInput();}
	inline void processEvents(F32 time){_scene->processEvents(time);}
	/*Base Scene Operations*/

	inline std::vector<FileData>& getModelDataArray() {return _scene->getModelDataArray();}
	inline std::vector<FileData>& getVegetationDataArray() {return _scene->getVegetationDataArray();}
    inline U32 getNumberOfTerrains(){return _scene->getNumberOfTerrains();}
   
	Scene* loadScene(const std::string& name);

	inline void addModel(FileData& model){_scene->addModel(model);}
	inline void addTerrain(TerrainDescriptor* ter) {_scene->addTerrain(ter);}
	void toggleBoundingBoxes();
	inline void addPatch(std::vector<FileData>& data){_scene->addPatch(data);}
	
	void findSelection(U32 x, U32 y);
	void deleteSelection();

	inline void clean(){_scene->clean();}
	void registerScene(Scene* scenePointer);
private:

	SceneManager();
	~SceneManager();
	Scene* _scene;
    Object3D* _currentSelection;
	unordered_map<std::string, Scene*> _sceneMap;

END_SINGLETON

inline void REGISTER_SCENE(Scene* const scene){
	SceneManager::getInstance().registerScene(scene);
}

#endif


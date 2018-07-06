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


#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H
#include "core.h"
#include "Scenes/Headers/Scene.h"

DEFINE_SINGLETON(SceneManager)

public:

	Scene* loadScene(const std::string& name);
	void   registerScene(Scene* scenePointer);

	inline Scene* getActiveScene()                   { return _scene; }
	inline void   setActiveScene(Scene* const scene) { SAFE_UPDATE(_scene, scene); }

	/*Base Scene Operations*/
	void render(RENDER_STAGE stage);
	inline void clean()                               { _scene->clean(); }
	inline bool unload()                              { return _scene->unload(); }
	inline bool load(const std::string& name)         { return _scene->load(name); }
	///Create AI entities, teams, NPC's etc
	inline bool initializeAI(bool continueOnErrors)   { bool state = _scene->initializeAI(continueOnErrors); 
													    _scene->_aiEvent.get()->startEvent();
														return state;
	                                                  }
	///Destroy all AI entities, teams, NPC's createa in "initializeAI" (AIEntities are deleted automatically by the AIManager
	inline bool deinitializeAI(bool continueOnErrors) { return _scene->deinitializeAI(continueOnErrors); }
	/// Update animations, network data, sounds, triggers etc.
	inline void updateCamera(Camera* const camera)  { _scene->updateCamera(camera); }
	inline void updateSceneState(D32 sceneTime)     { _scene->updateSceneState(sceneTime); }
	inline void preRender()                         { _scene->preRender(); }
	///Gather input events and process them in the current scene
	inline void processInput()                      { _scene->processInput(); }
	inline void processEvents(F32 time)             { _scene->processEvents(time); }
	/*Base Scene Operations*/

	inline U32 getNumberOfTerrains()                       { return _scene->getNumberOfTerrains(); }
	inline std::vector<FileData>& getModelDataArray()      { return _scene->getModelDataArray(); }
	inline std::vector<FileData>& getVegetationDataArray() { return _scene->getVegetationDataArray(); }
   
	
	inline void addModel(FileData& model)                    { _scene->addModel(model); }
	inline void addTerrain(TerrainDescriptor* const  ter)    { _scene->addTerrain(ter); }
	inline void addPatch(std::vector<FileData>& data)        { _scene->addPatch(data); }
	inline void toggleSkeletons()                            { _scene->drawSkeletons(!_scene->drawSkeletons()); }
		   void toggleBoundingBoxes();	

	void findSelection(const vec3<F32>& camOrigin, U32 x, U32 y);
	void deleteSelection();

	inline void cacheResolution(const vec2<U16>& newResolution) {_cachedResolution = newResolution; _scene->cacheResolution(newResolution);}
private:

	SceneManager();
	~SceneManager();
	Scene* _scene;
    Object3D* _currentSelection;
	typedef unordered_map<std::string, Scene*> SceneMap;
	SceneMap _sceneMap;
   ///cached resolution
   vec2<U16> _cachedResolution;

END_SINGLETON

inline void REGISTER_SCENE(Scene* const scene){
	SceneManager::getInstance().registerScene(scene);
}

inline Scene* GET_ACTIVE_SCENE() {
	return SceneManager::getInstance().getActiveScene();
}

#endif


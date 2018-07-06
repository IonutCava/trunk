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
#include <boost/functional/factory.hpp>

DEFINE_SINGLETON(SceneManager)

public:
	///Lookup the factory methods table and return the pointer to a newly constructed scene bound to that name
	Scene* createScene(const std::string& name);

	inline Scene* getActiveScene()                   { return _activeScene; }
	inline void   setActiveScene(Scene* const scene) { SAFE_UPDATE(_activeScene, scene); }

	/*Base Scene Operations*/
	void render(RENDER_STAGE stage);
	inline void idle()                                { _activeScene->idle(); }
	//inline bool unload()                              { return _activeScene->unload(); }
	bool load(const std::string& name, const vec2<U16>& resolution,  Camera* const camera);
	///Create AI entities, teams, NPC's etc
	inline bool initializeAI(bool continueOnErrors)   { return _activeScene->initializeAI(continueOnErrors); }
	///Destroy all AI entities, teams, NPC's createa in "initializeAI"
	///AIEntities are deleted automatically by the AIManager if they are not freed in "deinitializeAI"
	inline bool deinitializeAI(bool continueOnErrors) { return _activeScene->deinitializeAI(continueOnErrors); }
	/// Update animations, network data, sounds, triggers etc.
	inline void updateCamera(Camera* const camera)  { _activeScene->updateCamera(camera); }
	inline void updateSceneState(D32 sceneTime)     { _activeScene->updateSceneState(sceneTime); }
	inline void preRender()                         { _activeScene->preRender(); }
	///Gather input events and process them in the current scene
	inline void processInput()                      { _activeScene->processInput(); }
	inline void processEvents(F32 time)             { _activeScene->processEvents(time); }
	
	inline void cacheResolution(const vec2<U16>& newResolution) {_activeScene->cacheResolution(newResolution);}

	///Insert a new scene factory method for the give name
	template<class DerivedScene>
	inline bool registerScene(const std::string& sceneName) {
		_sceneFactory[sceneName] = boost::factory<DerivedScene*>();	
		return true;
	}

private:
	typedef unordered_map<std::string, Scene*> SceneMap;

	SceneManager();
	~SceneManager();

	///Pointer to the currently active scene
	Scene* _activeScene;
	///Scene pool
	SceneMap _sceneMap;
	///Scene_Name -Scene_Factory table
	unordered_map<std::string, boost::function<Scene*()> > _sceneFactory;

END_SINGLETON

///Return a pointer to the currently active scene
inline Scene* GET_ACTIVE_SCENE() {
	return SceneManager::getInstance().getActiveScene();
}

#endif


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

#ifndef _SCENE_H
#define _SCENE_H

#include "SceneState.h"
#include "Utility/Headers/Event.h"
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Core/Headers/ParamHandler.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Managers/Headers/LightManager.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Hardware/Input/Headers/InputInterface.h"

class Light;
class SceneGraph;
class TerrainDescriptor;

///The scene is a resource (to enforce load/unload and setName) and it has a state
///SceneState is added as an ancestor for accesibility reasons
class Scene : public Resource, public SceneState{

public:
	Scene() :  Resource(),
			   SceneState(),
			   _GFX(GFX_DEVICE),
			   _paramHandler(ParamHandler::getInstance()),
			   _lightTexture(NULL),
			   _deferredBuffer(NULL),
			   _deferredShader(NULL),
			   _camera(NULL),
			   _currentSelection(NULL),
			   _sceneGraph(New SceneGraph()) {}

	virtual ~Scene() {}

	SceneGraphNode* addGeometry(Object3D* const object);
	bool removeGeometry(SceneNode* node);

	/**Begin scene logic loop*/
	virtual void processInput() = 0;
	virtual void processEvents(F32 time) = 0;
	virtual void preRender() = 0;
	virtual void render() = 0;
	bool idle();
	/**End scene logic loop*/

	/// Update animations, network data, sounds, triggers etc.
	virtual void updateSceneState(D32 sceneTime);
	/// Update current camera (simple, fast, inlined poitner swap)
	inline void updateCamera(Camera* const camera) {_camera = camera;}

	inline std::vector<TerrainDescriptor*>& getTerrainInfoArray()    {return _terrainInfoArray;}
	inline std::vector<FileData>&           getModelDataArray()      {return _modelDataArray;}
	inline std::vector<FileData>&           getVegetationDataArray() {return _vegetationDataArray;}
	inline std::vector<Event_ptr>&          getEvents()              {return _events;}
	inline SceneGraph*						getSceneGraph()	         {return _sceneGraph;}
	inline ShaderProgram*					getDeferredShaders()     {return _deferredShader;}

	inline void addEvent(Event_ptr eventItem)          {_events.push_back(eventItem);}
	inline void addModel(FileData& model)              {_modelDataArray.push_back(model);}
	inline void addTerrain(TerrainDescriptor* ter)     {_terrainInfoArray.push_back(ter);}
	       void addPatch(std::vector<FileData>& data);
	       void addLight(Light* const lightItem);
	
	inline void cacheResolution(const vec2<U16>& newResolution) {_cachedResolution = newResolution;}

	///Object picking
	void findSelection(const vec3<F32>& camOrigin, U32 x, U32 y);
	void deleteSelection();

protected:
	///Global info
	GFXDevice&    _GFX;
	ParamHandler& _paramHandler;
	Camera*       _camera;
	SceneGraph*   _sceneGraph;

	///Datablocks for models,vegetation,terrains,events etc
	std::vector<F32>                _eventTimers;
	std::vector<FileData>           _modelDataArray;
	std::vector<FileData>           _vegetationDataArray;
	std::vector<FileData>           _pendingDataArray;
	std::vector<TerrainDescriptor*> _terrainInfoArray;
	
	///Deferred rendering
	FrameBufferObject* _deferredBuffer;
	PixelBufferObject* _lightTexture;
	ShaderProgram*	   _deferredShader;
 
	///cached resolution
    vec2<U16> _cachedResolution;
    ///Current selection
    SceneGraphNode* _currentSelection;

private: 
	std::vector<Event_ptr> _events;

protected:

	friend class SceneManager;
	/**Begin loading and unloading logic*/
	virtual bool preLoad() {_GFX.enableFog(_fogDensity,_fogColor);return true;}

	///Description in SceneManager
	virtual bool loadResources(bool continueOnErrors)  {return true;}
	virtual bool loadEvents(bool continueOnErrors)     {return true;}
	virtual void loadXMLAssets();
	virtual bool load(const std::string& name);
			bool loadModel(const FileData& data);
			bool loadGeometry(const FileData& data);
	virtual bool unload();
	///Description in SceneManager
	virtual bool initializeAI(bool continueOnErrors)   {return true;}
	///Description in SceneManager
	virtual bool deinitializeAI(bool continueOnErrors) {return true;}
	///End all tasks
	void clearEvents();
	///Unload scenegraph
	void clearObjects();
	///Destroy lights
	void clearLights();
	/**End loading and unloading logic*/

	Light* addDefaultLight();

public: //Input
	virtual void onKeyDown(const OIS::KeyEvent& key);
	virtual void onKeyUp(const OIS::KeyEvent& key);
	virtual void OnJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis);
	virtual void OnJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov){}
	virtual void OnJoystickButtonDown(const OIS::JoyStickEvent& key,I8 button){}
	virtual void OnJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){}
	virtual void onMouseMove(const OIS::MouseEvent& key){}
	virtual void onMouseClickDown(const OIS::MouseEvent& key,OIS::MouseButtonID button);
	virtual void onMouseClickUp(const OIS::MouseEvent& key,OIS::MouseButtonID button);
};

///usage: REGISTER_SCENE(A,B) where: - A is the scene's class name 
///									  -B is the name used to refer to that scene in the XML files
///Call this function after each scene declaration
#define REGISTER_SCENE_W_NAME(scene, sceneName) bool scene ## _registered = SceneManager::getInstance().registerScene<scene>(#sceneName);
///same as REGISTER_SCENE(A,B) but in this case the scene's name in XML must be the same as the class name
#define REGISTER_SCENE(scene) bool scene ## _registered = SceneManager::getInstance().registerScene<scene>(#scene);

#endif

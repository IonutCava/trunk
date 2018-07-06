/*“Copyright 2009-2013 DIVIDE-Studio”*/
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

#ifndef _SCENE_H_
#define _SCENE_H_

#include "SceneState.h"
#include "Hardware/Platform/Headers/Task.h"

class Sky;
class Light;
class Object3D;
class TerrainDescriptor;

/*All these includes are useful for a scene, so instead of forward declaring the classes, we include the headers
  to make them available in every scene source file. To reduce compile times, forward declare the "Scene" class instead
*/
//Core files
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
//Managers
#include "Managers/Headers/LightManager.h"
//Hardware
#include "Hardware/Video/Headers/GFXDevice.h"
#include "Hardware/Input/Headers/InputInterface.h"
//Scene Elements
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
//GUI
#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIElement.h"

///The scene is a resource (to enforce load/unload and setName) and it has a 2 states: one for game information and one for rendering information
class PhysicsSceneInterface;
class Scene : public Resource{
public:

	Scene();
	virtual ~Scene();

    SceneGraphNode* addGeometry(SceneNode* const object,const std::string& sgnName = "");
	bool removeGeometry(SceneNode* node);

	/**Begin scene logic loop*/
	virtual void processInput() = 0;                //<Get all input commands from the user
	virtual void processTasks(const U32 time) = 0;  //<Update the scene based on the inputs
	virtual void preRender() = 0;                   //<Prepare the scene for rendering after the update
    virtual void postRender();                      //<Perform any post rendering operations (such as showing texture previews)
	bool idle();                                    //<Scene is rendering, so add intensive tasks here to save CPU cycles
	/**End scene logic loop*/

	/// Update animations, network data, sounds, triggers etc.
	virtual void updateSceneState(const U32 sceneTime);
	inline SceneGraphNode*                 getSkySGN(I32 index)     {if(_skiesSGN.empty()) {return NULL;} CLAMP<I32>(index,0,_skiesSGN.size() - 1); return _skiesSGN[index];}
	inline vectorImpl<TerrainDescriptor*>& getTerrainInfoArray()    {return _terrainInfoArray;}
	inline vectorImpl<FileData>&           getModelDataArray()      {return _modelDataArray;}
	inline vectorImpl<FileData>&           getVegetationDataArray() {return _vegetationDataArray;}
	inline const vectorImpl<Task_ptr>&     getTasks()               {return _tasks;}
	inline SceneGraph*					   getSceneGraph()	        {return _sceneGraph;}
	inline SceneState*                     state()                  {return _sceneState;}
	inline SceneRenderState*               renderState()            {return _sceneRenderState;}
           void removeTasks();
           void removeTask(Task_ptr taskItem);
           void removeTask(U32 guid);
	       void addTask(Task_ptr taskItem);
	inline void addModel(FileData& model)              {_modelDataArray.push_back(model);}
	inline void addTerrain(TerrainDescriptor* ter)     {_terrainInfoArray.push_back(ter);}
	       void addPatch(vectorImpl<FileData>& data);
	       void addLight(Light* const lightItem);

	inline void cacheResolution(const vec2<U16>& newResolution) {_sceneRenderState->_cachedResolution = newResolution;}

	///Object picking
	void findSelection(const vec3<F32>& camOrigin, U32 x, U32 y);
	void deleteSelection();

	///call this function if you want to use a more complex rendering callback other than "SceneGraph::render()"
	void renderCallback(boost::function0<void> renderCallback) {_renderCallback = renderCallback;}
	boost::function0<void> renderCallback() {return _renderCallback;}

    ///Override this if you need a custom physics implementation (idle,update,process,etc)
    virtual PhysicsSceneInterface* createPhysicsImplementation();

protected:
	///Global info
	GFXDevice&    _GFX;
	ParamHandler& _paramHandler;
	SceneGraph*   _sceneGraph;

	PhysicsSceneInterface*         _physicsInterface;
	///Datablocks for models,vegetation,terrains,tasks etc
	vectorImpl<F32>                _taskTimers;
	vectorImpl<FileData>           _modelDataArray;
	vectorImpl<FileData>           _vegetationDataArray;
	vectorImpl<FileData>           _pendingDataArray;
	vectorImpl<TerrainDescriptor*> _terrainInfoArray;
	F32                            _FBSpeedFactor;
	F32                            _LRSpeedFactor;
    ///Current selection
    SceneGraphNode* _currentSelection;

	///This is the rendering function used to override the default one for the renderer.
	///If this is empty, the renderer will use the scene's scenegraph render function
	boost::function0<void> _renderCallback;

	///Scene::load must be called by every scene. Add a load flag to make sure!
	bool _loadComplete;

	///_aiTask is the thread handling the AIManager. It is started before each scene's "initializeAI" is called
	///It is destroyed after each scene's "deinitializeAI" is called
	std::tr1::shared_ptr<Task>  _aiTask;

private:
	vectorImpl<Task_ptr> _tasks;
	///saves all the rendering information for the scene (camera position, light info, draw states)
	SceneRenderState* _sceneRenderState;
	///Contains all game related info for the scene (wind speed, visibility ranges, etc)
	SceneState*      _sceneState;
	vectorImpl<SceneGraphNode* >   _skiesSGN;///<Add multiple skies that you can toggle through

protected:

	friend class SceneManager;
	/**Begin loading and unloading logic*/
	virtual bool preLoad();
	///Description in SceneManager
	virtual bool loadResources(bool continueOnErrors)  {return true;}
	virtual bool loadTasks(bool continueOnErrors)      {return true;}
    virtual bool loadPhysics(bool continueOnErrors);
	virtual void loadXMLAssets();
	virtual bool load(const std::string& name);
			bool loadModel(const FileData& data);
			bool loadGeometry(const FileData& data);
	virtual bool unload();
	///Description in SceneManager
	virtual bool initializeAI(bool continueOnErrors);
	///Description in SceneManager
	virtual bool deinitializeAI(bool continueOnErrors);
	///Check if Scene::load() was called
	bool checkLoadFlag() {return _loadComplete;}
	///End all tasks
	void clearTasks();
	///Unload scenegraph
	void clearObjects();
	///Destroy lights
	void clearLights();
    ///Destroy physics (:D)
    void clearPhysics();
	/**End loading and unloading logic*/

	Light* addDefaultLight();
	Sky*   addDefaultSky();

public: //Input
	virtual void onKeyDown(const OIS::KeyEvent& key);
	virtual void onKeyUp(const OIS::KeyEvent& key);
	virtual void onJoystickMoveAxis(const OIS::JoyStickEvent& key,I8 axis,I32 deadZone);
	virtual void onJoystickMovePOV(const OIS::JoyStickEvent& key,I8 pov){}
	virtual void onJoystickButtonDown(const OIS::JoyStickEvent& key,I8 button){}
	virtual void onJoystickButtonUp(const OIS::JoyStickEvent& key, I8 button){}
	virtual void sliderMoved( const OIS::JoyStickEvent &arg, I8 index){}
	virtual void vector3Moved( const OIS::JoyStickEvent &arg, I8 index){}
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
///simple macro to load the scene elements. Use "loadState' as loading flag
#define SCENE_LOAD(Name, ContOnErrorRes, ContOnErrorTasks) bool loadState = Scene::load(Name); \
											                if(loadState) loadState = loadResources(ContOnErrorRes); \
															if(loadState) loadState = loadTasks(ContOnErrorTasks); \
                                                            if(loadState) loadState = loadPhysics(ContOnErrorTasks); \
															if(!loadState) return false;

///Add all of the hardware headers in one:

#endif

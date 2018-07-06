/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SCENE_H_
#define _SCENE_H_

#include "SceneState.h"
#include "Core/Headers/cdigginsAny.h"
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
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Dynamics/Physics/Headers/PXDevice.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"
//GUI
#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIElement.h"

///The scene is a resource (to enforce load/unload and setName) and it has a 2 states: one for game information and one for rendering information
class ParticleEmitter;
struct ParticleEmitterDescriptor;
class PhysicsSceneInterface;
class Scene : public Resource, public InputAggregatorInterface {
protected:
    typedef std::stack<FileData, vectorImpl<FileData> > FileDataStack;
#ifdef _DEBUG
    enum DebugLines{
        DEBUG_LINE_RAY_PICK = 0,
        DEBUG_LINE_OBJECT_TO_TARGET = 1,
        DEBUG_LINE_PLACEHOLDER
    };
#endif
public:

    Scene();
    virtual ~Scene();

    SceneGraphNode* addGeometry(SceneNode* const object,const std::string& sgnName = "");
    bool removeGeometry(SceneNode* node);

    /**Begin scene logic loop*/
    virtual void processInput(const U64 deltaTime) = 0;  //<Get all input commands from the user
    virtual void processTasks(const U64 deltaTime); //<Update the scene based on the inputs
    virtual void processGUI(const U64 deltaTime);
    virtual void preRender() {}                       //<Prepare the scene for rendering after the update
    virtual void postRender();                      //<Perform any post rendering operations
    bool idle();                                    //<Scene is rendering, so add intensive tasks here to save CPU cycles
    void onLostFocus();                             //<The application has lost focus
    /**End scene logic loop*/

    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTime);
    /// Override this for Scene specific updates
    virtual void updateSceneStateInternal(const U64 deltaTime)  {}
    inline SceneGraphNode*                 getSkySGN(I32 index)     {if(_skiesSGN.empty()) {return nullptr;} CLAMP<I32>(index,0,(I32)_skiesSGN.size() - 1); return _skiesSGN[index];}
    inline const vectorImpl<Task_ptr>&     getTasks()               {return _tasks;}
    inline SceneState&                     state()                  {return _sceneState;}
    inline SceneRenderState&               renderState()            {return _sceneState.getRenderState();}
    inline SceneGraph*					   getSceneGraph()	        {return _sceneGraph;}

           void clearTasks();
           void removeTask(Task_ptr taskItem);
           void removeTask(U32 guid);
           void addTask(Task_ptr taskItem);
    inline void addModel(FileData& model)              {_modelDataArray.push(model);}
    inline void addTerrain(TerrainDescriptor* ter)     {_terrainInfoArray.push_back(ter);}
           void addPatch(vectorImpl<FileData>& data);
           SceneGraphNode* addLight(Light* const lightItem, SceneGraphNode* const parentNode = nullptr);

    inline void cacheResolution(const vec2<U16>& newResolution) {
        _sceneState.getRenderState()._cachedResolution = newResolution;
    }

    ///Object picking
    inline SceneGraphNode* getCurrentSelection() const { return _currentSelection; }
    void findSelection(F32 mouseX, F32 mouseY);
    void deleteSelection();
    inline void addSelectionCallback(const DELEGATE_CBK& selectionCallback) { 
        _selectionChangeCallbacks.push_back(selectionCallback); 
    }
    
    ///call this function if you want to use a more complex rendering callback other than "SceneGraph::render()"
    void renderCallback(const DELEGATE_CBK& renderCallback) {_renderCallback = renderCallback;}
    const DELEGATE_CBK& renderCallback() {return _renderCallback;}

    ///Override this if you need a custom physics implementation (idle,update,process,etc)
    virtual PhysicsSceneInterface* createPhysicsImplementation();

    ParticleEmitter* addParticleEmitter(const std::string& name, const ParticleEmitterDescriptor& descriptor);
    ParticleEmitter* getParticleEmitter(const std::string& name);

    TerrainDescriptor* getTerrainInfo(const std::string& terrainName);
    inline vectorImpl<FileData>& getVegetationDataArray() { return _vegetationDataArray; }


protected:
    ///Global info
    GFXDevice&     _GFX;
    GUI*           _GUI;
    ParamHandler&  _paramHandler;
    SceneGraph*    _sceneGraph;
    
    PhysicsSceneInterface*         _physicsInterface;
    vectorImpl<D32>                _taskTimers;
    vectorImpl<D32>                _guiTimers;
    ///Datablocks for models,vegetation,terrains,tasks etc
    FileDataStack                  _modelDataArray;
    vectorImpl<FileData>           _vegetationDataArray;
    vectorImpl<TerrainDescriptor*> _terrainInfoArray;
    F32                            _LRSpeedFactor;
    ///Current selection
    SceneGraphNode* _currentSelection;

    ///This is the rendering function used to override the default one for the renderer.
    ///If this is empty, the renderer will use the scene's scenegraph render function
    DELEGATE_CBK _renderCallback;

    ///Scene::load must be called by every scene. Add a load flag to make sure!
    bool _loadComplete;
    ///Schedule a scene graph parse with the physics engine to recreate/recheck the collision meshes used by each node
    bool _cookCollisionMeshesScheduled;
    ///_aiTask is the thread handling the AIManager. It is started before each scene's "initializeAI" is called
    ///It is destroyed after each scene's "deinitializeAI" is called
    std::shared_ptr<Task>  _aiTask;

    typedef Unordered_map<std::string, ParticleEmitter *> ParticleEmitterMap;
    ParticleEmitterMap _particleEmitters;

private:
    vectorImpl<Task_ptr> _tasks;
    ///Contains all game related info for the scene (wind speed, visibility ranges, etc)
    SceneState       _sceneState;
    vectorImpl<SceneGraphNode* >   _skiesSGN;///<Add multiple skies that you can toggle through
    vectorImpl<DELEGATE_CBK > _selectionChangeCallbacks;

protected:

    friend class SceneManager;
    /**Begin loading and unloading logic*/
    virtual bool preLoad();
    ///Description in SceneManager
    virtual bool loadResources(bool continueOnErrors)  {return true;}
    virtual bool loadTasks(bool continueOnErrors)      {return true;}
    virtual bool loadPhysics(bool continueOnErrors);
    ///if singleStep is true, only the first model from the modelArray will be loaded. 
    ///Useful for loading one model per frame
    virtual void loadXMLAssets(bool singleStep = false);
    virtual bool load(const std::string& name, CameraManager* const cameraMgr, GUI* const guiInterface);
            bool loadModel(const FileData& data);
            bool loadGeometry(const FileData& data);
    virtual bool unload();
    ///Description in SceneManager
    virtual bool initializeAI(bool continueOnErrors);
    ///Description in SceneManager
    virtual bool deinitializeAI(bool continueOnErrors);
    ///Check if Scene::load() was called
    bool checkLoadFlag() const {return _loadComplete;}
    ///Unload scenegraph
    void clearObjects();
    ///Destroy lights
    void clearLights();
    ///Destroy physics (:D)
    void clearPhysics();
    /**End loading and unloading logic*/
    ///This is a camera listener. Do not call directly.
    void onCameraChange();
    /// returns true if the camera was moved/rotated/etc
    bool updateCameraControls();
    /// Draw debug entities
    void debugDraw(const RenderStage& stage);

    Sky*               addDefaultSky();
    DirectionalLight*  addDefaultLight();
    
    ///simple function to load the scene elements.
    inline bool SCENE_LOAD(const std::string& name, CameraManager* const cameraMgr, GUI* const gui, const bool contOnErrorRes, const bool contOnErrorTasks){
        if(!Scene::load(name,cameraMgr,gui)) {
            ERROR_FN(Locale::get("ERROR_SCENE_LOAD"), "scene load function");
            return false;
        }
        if(!loadResources(contOnErrorRes)){
            ERROR_FN(Locale::get("ERROR_SCENE_LOAD"), "scene load resources");
            if(!contOnErrorRes) return false;
        }
        if(!loadTasks(contOnErrorTasks)){
            ERROR_FN(Locale::get("ERROR_SCENE_LOAD"), "scene load tasks");
            if(!contOnErrorTasks) return false;
        }
        if(!loadPhysics(contOnErrorTasks)){
            ERROR_FN(Locale::get("ERROR_SCENE_LOAD"), "scene load physics");
            if(!contOnErrorTasks) return false;
        }
        return true;
    }

    void defaultCameraKeys(OIS::KeyCode code, bool upState);

public: //Input
    virtual bool onKeyDown(const OIS::KeyEvent& key);
    virtual bool onKeyUp(const OIS::KeyEvent& key);
    virtual bool joystickAxisMoved(const OIS::JoyStickEvent& key,I8 axis);
    virtual bool joystickPovMoved(const OIS::JoyStickEvent& key, I8 pov);
    virtual bool joystickButtonPressed(const OIS::JoyStickEvent& key,I8 button){ return true;}
    virtual bool joystickButtonReleased(const OIS::JoyStickEvent& key, I8 button) { return true; }
    virtual bool joystickSliderMoved( const OIS::JoyStickEvent &arg, I8 index) { return true; }
    virtual bool joystickVector3DMoved( const OIS::JoyStickEvent &arg, I8 index){ return true; }
    virtual bool mouseMoved(const OIS::MouseEvent& key);
    virtual bool mouseButtonPressed(const OIS::MouseEvent& key,OIS::MouseButtonID button);
    virtual bool mouseButtonReleased(const OIS::MouseEvent& key,OIS::MouseButtonID button);

protected: //Input
    vec2<F32> _previousMousePos;
    bool _mousePressed[8];
#ifdef _DEBUG
    vectorImpl<Line > _lines[DEBUG_LINE_PLACEHOLDER];
#endif
};

///usage: REGISTER_SCENE(A,B) where: - A is the scene's class name
///									  -B is the name used to refer to that scene in the XML files
///Call this function after each scene declaration
#define REGISTER_SCENE_W_NAME(scene, sceneName) bool scene ## _registered = SceneManager::getOrCreateInstance().registerScene<scene>(#sceneName);
///same as REGISTER_SCENE(A,B) but in this case the scene's name in XML must be the same as the class name
#define REGISTER_SCENE(scene) bool scene ## _registered = SceneManager::getOrCreateInstance().registerScene<scene>(#scene);

#endif

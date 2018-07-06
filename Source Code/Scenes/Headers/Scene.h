/*
   Copyright (c) 2016 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _SCENE_H_
#define _SCENE_H_

#include "SceneState.h"
#include "SceneInput.h"
#include "Core/Headers/cdigginsAny.h"
#include "Platform/Threading/Headers/Task.h"

/*All these includes are useful for a scene, so instead of forward declaring the
  classes, we include the headers
  to make them available in every scene source file. To reduce compile times,
  forward declare the "Scene" class instead
*/
// Core files
#include "Core/Headers/Kernel.h"
#include "Core/Headers/Application.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
// Managers
#include "Managers/Headers/LightManager.h"
// Hardware
#include "Platform/Video/Headers/GFXDevice.h"
#include "Platform/Input/Headers/InputInterface.h"
// Scene Elements
#include "Environment/Sky/Headers/Sky.h"
#include "Rendering/Lighting/Headers/Light.h"
#include "Rendering/Lighting/Headers/DirectionalLight.h"
#include "Physics/Headers/PXDevice.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"
// GUI
#include "GUI/Headers/GUI.h"
#include "GUI/Headers/GUIElement.h"

namespace Divide {
class Sky;
class Light;
class Object3D;
class LoadSave;
class TerrainDescriptor;
class ParticleEmitter;
class PhysicsSceneInterface;

namespace Attorney {
    class SceneManager;
    class SceneGraph;
    class SceneRenderPass;
    class SceneLoadSave;
};

/// The scene is a resource (to enforce load/unload and setName) and it has a 2
/// states:
/// one for game information and one for rendering information
class NOINITVTABLE Scene : public Resource {
    friend class Attorney::SceneManager;
    friend class Attorney::SceneGraph;
    friend class Attorney::SceneRenderPass;
    friend class Attorney::SceneLoadSave;

   protected:
    typedef std::stack<FileData, vectorImpl<FileData> > FileDataStack;

   public:
    Scene();
    virtual ~Scene();

    /**Begin scene logic loop*/
    /// Get all input commands from the user
    virtual void processInput(const U64 deltaTime);
    /// Update the scene based on the inputs
    virtual void processTasks(const U64 deltaTime);
    virtual void processGUI(const U64 deltaTime);
    /// Scene is rendering, so add intensive tasks here to save CPU cycles
    bool idle();  
    /// The application has lost focus
    void onLostFocus();  
    /**End scene logic loop*/

    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTime);
    /// Override this for Scene specific updates
    virtual void updateSceneStateInternal(const U64 deltaTime) {}
    inline SceneState& state() { return _sceneState; }
    inline const SceneState& state() const { return _sceneState; }
    inline SceneRenderState& renderState() { return _sceneState.renderState(); }
    inline const SceneRenderState& renderState() const { return _sceneState.renderState(); }
    inline SceneInput& input() { return *_input; }

    inline SceneGraph& getSceneGraph() { return _sceneGraph; }
    void registerTask(const TaskHandle& taskItem);
    void clearTasks();
    void removeTask(I64 jobIdentifier);

    inline void addModel(FileData& model) { _modelDataArray.push(model); }
    inline void addTerrain(TerrainDescriptor* ter) {
        _terrainInfoArray.push_back(ter);
    }
    void addPatch(vectorImpl<FileData>& data);

    // DIRECTIONAL lights have shadow mapping enabled automatically
    SceneGraphNode_ptr addLight(LightType type, SceneGraphNode& parentNode);
    SceneGraphNode_ptr addSky();
    SceneGraphNode_ptr addSky(Sky& skyItem);

    /// Object picking
    inline SceneGraphNode_wptr getCurrentSelection() const {
        return _currentSelection;
    }
    inline SceneGraphNode_wptr getCurrentHoverTarget() const {
        return  _currentHoverTarget;
    }
    void findSelection();

    inline void addSelectionCallback(const DELEGATE_CBK<>& selectionCallback) {
        _selectionChangeCallbacks.push_back(selectionCallback);
    }

    /// Override this if you need a custom physics implementation
    /// (idle,update,process,etc)
    virtual PhysicsSceneInterface* createPhysicsImplementation();

    SceneGraphNode_ptr addParticleEmitter(const stringImpl& name,
                                          std::shared_ptr<ParticleData> data,
                                          SceneGraphNode& parentNode);

    TerrainDescriptor* getTerrainInfo(const stringImpl& terrainName);
    inline vectorImpl<FileData>& getVegetationDataArray() {
        return _vegetationDataArray;
    }

   protected:
    /// Global info
    GFXDevice& _GFX;
    GUI* _GUI;
    ParamHandler& _paramHandler;
    SceneGraph _sceneGraph;

    U64 _sceneTimer;
    vectorImpl<D64> _taskTimers;
    vectorImpl<D64> _guiTimers;
    /// Datablocks for models,vegetation,terrains,tasks etc
    FileDataStack _modelDataArray;
    vectorImpl<FileData> _vegetationDataArray;
    vectorImpl<TerrainDescriptor*> _terrainInfoArray;
    F32 _LRSpeedFactor;
    /// Current selection
    SceneGraphNode_wptr _currentSelection;
    SceneGraphNode_wptr _currentHoverTarget;
    SceneGraphNode_wptr _currentSky;
    SceneGraphNode_wptr _flashLight;

    /// Scene::load must be called by every scene. Add a load flag to make sure!
    bool _loadComplete;
    /// Schedule a scene graph parse with the physics engine to recreate/recheck
    /// the collision meshes used by each node
    bool _cookCollisionMeshesScheduled;
    ///_aiTask is the thread handling the AIManager. It is started before each scene's "initializeAI" is called
    /// It is destroyed after each scene's "deinitializeAI" is called
    std::thread _aiTask;

   private:
    vectorImpl<TaskHandle> _tasks;
    /// Contains all game related info for the scene (wind speed, visibility
    /// ranges, etc)
    SceneState _sceneState;
    vectorImpl<DELEGATE_CBK<> > _selectionChangeCallbacks;
    vectorImpl<SceneGraphNode_cwptr> _sceneSelectionCandidates;

   protected:
    void resetSelection();
    void findHoverTarget();
    void toggleFlashlight();

    virtual bool save(ByteBuffer& outputBuffer) const;
    virtual bool load(ByteBuffer& inputBuffer);

    virtual bool frameStarted();
    virtual bool frameEnded();
    /// Description in SceneManager
    virtual bool loadResources(bool continueOnErrors) { return true; }
    virtual bool loadTasks(bool continueOnErrors) { return true; }
    virtual bool loadPhysics(bool continueOnErrors);
    /// if singleStep is true, only the first model from the modelArray will be
    /// loaded.
    /// Useful for loading one model per frame
    virtual void loadXMLAssets(bool singleStep = false);
    virtual bool load(const stringImpl& name, GUI* const guiInterface);
    bool loadModel(const FileData& data);
    bool loadGeometry(const FileData& data);
    virtual bool unload();
    virtual void postLoad();
    /// Description in SceneManager
    virtual bool initializeAI(bool continueOnErrors);
    virtual bool deinitializeAI(bool continueOnErrors);
    /// Check if Scene::load() was called
    bool checkLoadFlag() const { return _loadComplete; }
    /// Unload scenegraph
    void clearObjects();
    /**End loading and unloading logic*/
    /// returns true if the camera was moved/rotated/etc
    bool updateCameraControls();
    /// Draw debug entities
    void debugDraw(RenderStage stage);

    /// simple function to load the scene elements.
    inline bool SCENE_LOAD(const stringImpl& name, GUI* const gui,
                           const bool contOnErrorRes,
                           const bool contOnErrorTasks) {
        if (!Scene::load(name, gui)) {
            Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")),
                             "scene load function");
            return false;
        }
        if (!loadResources(contOnErrorRes)) {
            Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")),
                             "scene load resources");
            if (!contOnErrorRes) return false;
        }
        if (!loadTasks(contOnErrorTasks)) {
            Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")),
                             "scene load tasks");
            if (!contOnErrorTasks) return false;
        }
        if (!loadPhysics(contOnErrorTasks)) {
            Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")),
                             "scene load physics");
            if (!contOnErrorTasks) return false;
        }
        return true;
    }

   protected:
    std::unique_ptr<SceneInput> _input;
#ifdef _DEBUG
    IMPrimitive* _linesPrimitive;
    vectorImpl<IMPrimitive*> _octreePrimitives;
    vectorImpl<BoundingBox> _octreeBoundingBoxes;
#endif
};

namespace Attorney {
class SceneManager {
   private:
    static bool updateCameraControls(Scene& scene) {
        return scene.updateCameraControls();
    }
    static bool checkLoadFlag(Scene& scene) { return scene.checkLoadFlag(); }
    static bool initializeAI(Scene& scene, bool continueOnErrors) {
        return scene.initializeAI(continueOnErrors);
    }
    static bool deinitializeAI(Scene& scene) {
        return scene.deinitializeAI(true);
    }

    static bool frameStarted(Scene& scene) { return scene.frameStarted(); }
    static bool frameEnded(Scene& scene) { return scene.frameEnded(); }
    static bool load(Scene& scene, const stringImpl& name,
                     GUI* const guiInterface) {
        return scene.load(name, guiInterface);
    }
    static bool unload(Scene& scene) { return scene.unload(); }

    static void postLoad(Scene& scene) {
        scene.postLoad();
    }

    friend class Divide::SceneManager;
};

class SceneRenderPass {
 private:
    /// Draw debug entities
    static void debugDraw(Scene& scene, RenderStage stage) {
        scene.debugDraw(stage);
    }
    friend class Divide::RenderPass;
};

class SceneLoadSave {
 private:
    static bool save(const Scene& scene, ByteBuffer& outputBuffer) {
        return scene.save(outputBuffer);
    }

    static bool load(Scene& scene, ByteBuffer& inputBuffer) {
        return scene.load(inputBuffer);
    }

    friend class Divide::LoadSave;
};

class SceneGraph {
private:
    static void onNodeDestroy(Scene& scene, SceneGraphNode& node) {
        SceneGraphNode_ptr currentSelection = scene.getCurrentSelection().lock();
        if (currentSelection && currentSelection->getGUID() == node.getGUID()) {
            scene.resetSelection();
        }
    }
    friend class Divide::SceneGraph;
};
};  // namespace Attorney
};  // namespace Divide

/// usage: REGISTER_SCENE(A,B) where: - A is the scene's class name
///                                    -B is the name used to refer to that
///                                      scene in the XML files
/// Call this function after each scene declaration
#define REGISTER_SCENE_W_NAME(scene, sceneName) \
    bool g_registered_##scene = SceneFactory::instance().registerScene<scene>(_ID(sceneName));
/// same as REGISTER_SCENE(A,B) but in this case the scene's name in XML must be the same as the class name
#define REGISTER_SCENE(scene) REGISTER_SCENE_W_NAME(scene, #scene)

#endif

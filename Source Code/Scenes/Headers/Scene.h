/*
   Copyright (c) 2018 DIVIDE-Studio
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
#include "Platform/Threading/Headers/Task.h"

/*All these includes are useful for a scene, so instead of forward declaring the
  classes, we include the headers
  to make them available in every scene source file. To reduce compile times,
  forward declare the "Scene" class instead
*/

#include "GUI/Headers/GUI.h"
#include "GUI/Headers/SceneGUIElements.h"
#include "Graphs/Headers/SceneGraph.h"
#include "Rendering/Camera/Headers/Camera.h"
#include "Rendering/Lighting/Headers/LightPool.h"
#include "AI/Headers/AIManager.h"
#include "Physics/Headers/PXDevice.h"
#include "SceneEnvironmentProbePool.h"
#include "Platform/Video/Headers/GFXDevice.h"
#include "Environment/Sky/Headers/Sky.h"
#include "Dynamics/Entities/Particles/Headers/ParticleEmitter.h"
#include "Utility/Headers/XMLParser.h"

#include <ArenaAllocator/arena_allocator.h>

namespace Divide {
class Sky;
class Light;
class Object3D;
class LoadSave;
class ParamHandler;
class TerrainDescriptor;
class ParticleEmitter;
class PhysicsSceneInterface;

FWD_DECLARE_MANAGED_CLASS(Mesh);
FWD_DECLARE_MANAGED_CLASS(Player);

namespace Attorney {
    class SceneManager;
    class SceneGraph;
    class SceneRenderPass;
    class SceneLoadSave;
    class SceneGUI;
    class SceneInput;
};

/// The scene is a resource (to enforce load/unload and setName) and it has a 2 states:
/// one for game information and one for rendering information

class Scene : public Resource, public PlatformContextComponent {
    friend class Attorney::SceneManager;
    friend class Attorney::SceneGraph;
    friend class Attorney::SceneRenderPass;
    friend class Attorney::SceneLoadSave;
    friend class Attorney::SceneGUI;
    friend class Attorney::SceneInput;

   protected:
    static bool onStartup();
    static bool onShutdown();

   public:
    explicit Scene(PlatformContext& context, ResourceCache& cache, SceneManager& parent, const stringImpl& name);
    virtual ~Scene();

    /**Begin scene logic loop*/
    /// Get all input commands from the user
    virtual void processInput(PlayerIndex idx, const U64 deltaTimeUS);
    /// Update the scene based on the inputs
    virtual void processTasks(const U64 deltaTimeUS);
    virtual void processGUI(const U64 deltaTimeUS);
    /// Scene is rendering, so add intensive tasks here to save CPU cycles
    bool idle();  
    /// The application has lost focus
    void onLostFocus();  
    /**End scene logic loop*/

    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTimeUS);
    void onStartUpdateLoop(const U8 loopNumber);
    /// Override this for Scene specific updates
    virtual void updateSceneStateInternal(const U64 deltaTimeUS) { ACKNOWLEDGE_UNUSED(deltaTimeUS); }
    inline SceneState& state() { return *_sceneState; }
    inline const SceneState& state() const { return *_sceneState; }
    inline SceneRenderState& renderState() { return _sceneState->renderState(); }
    inline const SceneRenderState& renderState() const { return _sceneState->renderState(); }
    inline SceneInput& input() { return *_input; }
    inline const SceneInput& input() const { return *_input; }

    inline SceneGraph& sceneGraph() { return *_sceneGraph; }
    inline const SceneGraph& sceneGraph() const { return *_sceneGraph; }

    void registerTask(const TaskHandle& taskItem, bool start = true, TaskPriority priority = TaskPriority::DONT_CARE);
    void clearTasks();
    void removeTask(TaskHandle& task);

    inline void addSceneGraph(const XML::SceneNode& sceneGraph) { _xmlSceneGraph.push(sceneGraph); }
    inline void addTerrain(const std::shared_ptr<TerrainDescriptor>& ter) { _terrainInfoArray.push_back(ter); }
    void addMusic(MusicType type, const stringImpl& name, const stringImpl& srcFile);

    // DIRECTIONAL lights have shadow mapping enabled automatically
    SceneGraphNode* addLight(LightType type, SceneGraphNode& parentNode, stringImpl nodeName = "");
    SceneGraphNode* addSky(SceneGraphNode& parentNode, const stringImpl& nodeName = "");

    /// Object picking
    inline vector<I64> getCurrentSelection(PlayerIndex index = 0) const {
        auto it = _currentSelection.find(index);
        if (it != eastl::cend(_currentSelection)) {
            return it->second;
        }

        return {};
    }

    void findSelection(PlayerIndex idx, bool clearOld);

    inline void addSelectionCallback(const DELEGATE_CBK<void, U8, SceneGraphNode*>& selectionCallback) {
        _selectionChangeCallbacks.push_back(selectionCallback);
    }

    SceneGraphNode* addParticleEmitter(const stringImpl& name,
                                          std::shared_ptr<ParticleData> data,
                                          SceneGraphNode& parentNode);

    inline AI::AIManager& aiManager() { return *_aiManager; }
    inline const AI::AIManager& aiManager() const { return *_aiManager; }

    inline ResourceCache& resourceCache() { return _resCache; }
    inline const ResourceCache& resourceCache() const { return _resCache; }

    Camera* playerCamera() const;
    Camera* playerCamera(U8 index) const;

    inline LightPool& lightPool() { return *_lightPool; }
    inline const LightPool& lightPool() const { return *_lightPool; }

   protected:
    virtual void rebuildShaders();
    virtual void onSetActive();
    virtual void onRemoveActive();
    // returns the first available action ID
    virtual U16 registerInputActions();
    virtual void loadKeyBindings();

    void onNodeDestroy(SceneGraphNode& node);
    void findHoverTarget(PlayerIndex idx, bool force);
    bool checkCameraUnderwater(PlayerIndex idx) const;
    void toggleFlashlight(PlayerIndex idx);

    virtual bool save(ByteBuffer& outputBuffer) const;
    virtual bool load(ByteBuffer& inputBuffer);

    virtual bool frameStarted();
    virtual bool frameEnded();


    virtual void loadDefaultCamera();


    virtual bool loadXML(const stringImpl& name);
    virtual void saveXML();

    virtual bool load(const stringImpl& name);
    virtual void loadXMLData();
    void loadAsset(const XML::SceneNode& sceneNode, SceneGraphNode* parent);
    virtual bool unload();
    virtual void postLoad();
    // gets called on the main thread when the scene finishes loading (e.g. used by the GUI system)
    virtual void postLoadMainThread();
    /// Check if Scene::load() was called
    bool checkLoadFlag() const { return _loadComplete; }
    /// Unload scenegraph
    void clearObjects();
    /**End loading and unloading logic*/
    /// returns true if the camera was moved/rotated/etc
    bool updateCameraControls(PlayerIndex idx);
    /// Draw debug entities
    virtual void debugDraw(const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);

    //Return true if input was consumed
    virtual bool mouseMoved(const Input::MouseMoveEvent& arg);
    
    U8 getSceneIndexForPlayer(PlayerIndex idx) const;
    const Player_ptr& getPlayerForIndex(PlayerIndex idx) const;

    U8 getPlayerIndexForDevice(U8 deviceIndex) const;

    void addPlayerInternal(bool queue);
    void removePlayerInternal(PlayerIndex idx);
    void onPlayerAdd(const Player_ptr& player);
    void onPlayerRemove(const Player_ptr& player);

    /// simple function to load the scene elements.
    inline bool SCENE_LOAD(const stringImpl& name) {
        if (!Scene::load(name)) {
            Console::errorfn(Locale::get(_ID("ERROR_SCENE_LOAD")), "scene load function");
            return false;
        }

        registerInputActions();
        loadKeyBindings();

        return true;
    }

    stringImpl getPlayerSGNName(PlayerIndex idx);

    void currentPlayerPass(PlayerIndex idx);

    void resetSelection(PlayerIndex idx);
    void setSelected(PlayerIndex idx, SceneGraphNode& sgn);

    bool editorVisible() const;
   protected:
       /// Global info
       SceneManager& _parent;
       ResourceCache& _resCache;
       ParamHandler&  _paramHandler;
       SceneGraph*    _sceneGraph;
       AI::AIManager* _aiManager;
       SceneGUIElements* _GUI;

       SceneGraphNode* _sun;

       vector<Player_ptr> _scenePlayers;
       U64 _sceneTimerUS;
       vector<D64> _taskTimers;
       vector<D64> _guiTimersMS;
       /// Datablocks for models,vegetation,terrains,tasks etc
       std::atomic_uint _loadingTasks;
       std::stack<XML::SceneNode> _xmlSceneGraph;

       vector<std::shared_ptr<TerrainDescriptor>> _terrainInfoArray;
       F32 _LRSpeedFactor;
       /// Current selection
       hashMap<PlayerIndex, vector<I64>> _currentSelection;
       hashMap<PlayerIndex, I64> _currentHoverTarget;

       SceneGraphNode* _currentSky;
       hashMap<PlayerIndex, SceneGraphNode*> _flashLight;
       hashMap<PlayerIndex, U32> _cameraUpdateMap;
       /// Scene::load must be called by every scene. Add a load flag to make sure!
       bool _loadComplete;
       /// Schedule a scene graph parse with the physics engine to recreate/recheck
       /// the collision meshes used by each node
       bool _cookCollisionMeshesScheduled;

   private:
       SharedMutex _tasksMutex;
       vector<TaskHandle> _tasks;
       /// Contains all game related info for the scene (wind speed, visibility ranges, etc)
       SceneState* _sceneState;
       vector<DELEGATE_CBK<void, U8 /*player index*/, SceneGraphNode* /*node*/> > _selectionChangeCallbacks;
       vector<SGNRayResult> _sceneSelectionCandidates;

       std::unordered_set<PlayerIndex> _hoverUpdateQueue;

   protected:
       LightPool* _lightPool;
       SceneInput* _input;
       PhysicsSceneInterface* _pxScene;
       SceneEnvironmentProbePool* _envProbePool;

       IMPrimitive* _linesPrimitive;
       vector<IMPrimitive*> _octreePrimitives;
       vector<BoundingBox> _octreeBoundingBoxes;

       mutable std::mutex _perFrameArenaMutex;
       mutable MyArena<Config::REQUIRED_RAM_SIZE / 2> _perFrameArena;
};

namespace Attorney {
class SceneManager {
   private:
    static bool updateCameraControls(Scene& scene, PlayerIndex idx) {
        return scene.updateCameraControls(idx);
    }

    static bool checkLoadFlag(const Scene& scene) {
        return scene.checkLoadFlag();
    }

    static void onPlayerAdd(Scene& scene, const Player_ptr& player) {
        scene.onPlayerAdd(player);
    }

    static void onPlayerRemove(Scene& scene, const Player_ptr& player) {
        scene.onPlayerRemove(player);
    }

    static void currentPlayerPass(Scene& scene, PlayerIndex idx) {
        scene.currentPlayerPass(idx);
    }

    /// Draw debug entities
    static void debugDraw(Scene& scene, const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut) {
        scene.debugDraw(activeCamera, stagePass, bufferInOut);
    }

    static bool frameStarted(Scene& scene) { return scene.frameStarted(); }
    static bool frameEnded(Scene& scene) { return scene.frameEnded(); }

    static bool loadXML(Scene& scene, const stringImpl& name) {
        return scene.loadXML(name);
    }
    static bool load(Scene& scene, const stringImpl& name) {
        return scene.load(name);
    }

    static bool unload(Scene& scene) { 
        return scene.unload();
    }

    static void postLoad(Scene& scene) {
        scene.postLoad();
    }

    static void postLoadMainThread(Scene& scene) {
        scene.postLoadMainThread();
    }

    static void onSetActive(Scene& scene) {
        scene.onSetActive();
    }

    static void onRemoveActive(Scene& scene) {
        scene.onRemoveActive();
    }

    static bool onStartup() {
        return Scene::onStartup();
    }

    static bool onShutdown() {
        return Scene::onShutdown();
    }

    static SceneGUIElements* gui(Scene& scene) {
        return scene._GUI;
    }

    static void resetSelection(Scene& scene, PlayerIndex idx) {
        scene.resetSelection(idx);
    }

    static void setSelected(Scene& scene, PlayerIndex idx, SceneGraphNode& sgn) {
        scene.setSelected(idx, sgn);
    }

    friend class Divide::SceneManager;
};

class SceneRenderPass {
 private:
    static SceneEnvironmentProbePool* getEnvProbes(Scene& scene) {
        return scene._envProbePool;
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
        scene.onNodeDestroy(node);
    }

    friend class Divide::SceneGraph;
};

class SceneGUI {
private:
    static SceneGUIElements* guiElements(Scene& scene) {
        return scene._GUI;
    }

    friend class Divide::GUI;
};

class SceneInput {
private:
    static bool mouseMoved(Scene& scene, const Input::MouseMoveEvent& arg) {
        return scene.mouseMoved(arg);
    }

    friend class Divide::SceneInput;
};

};  // namespace Attorney
};  // namespace Divide

#endif
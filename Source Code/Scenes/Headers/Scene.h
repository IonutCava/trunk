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

#pragma once
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

struct Selections {
    static constexpr U8 MaxSelections = 254u;
    std::array<I64, MaxSelections> _selections;
    U8 _selectionCount = 0u;
};

struct DragSelectData {
    Rect<I32> _targetViewport;
    vec2<I32> _startDragPos;
    vec2<I32> _endDragPos;
    bool _isDragging = false;
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
    explicit Scene(PlatformContext& context, ResourceCache* cache, SceneManager& parent, const Str128& name);
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
    void onGainFocus();
    /**End scene logic loop*/

    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTimeUS);
    void onStartUpdateLoop(const U8 loopNumber);
    /// Override this for Scene specific updates
    virtual void updateSceneStateInternal(const U64 deltaTimeUS) { ACKNOWLEDGE_UNUSED(deltaTimeUS); }
    inline SceneState& state() noexcept { return *_sceneState; }
    inline const SceneState& state() const noexcept { return *_sceneState; }
    inline SceneRenderState& renderState() { return _sceneState->renderState(); }
    inline const SceneRenderState& renderState() const { return _sceneState->renderState(); }
    inline SceneInput& input() noexcept { return *_input; }
    inline const SceneInput& input() const noexcept { return *_input; }

    inline SceneGraph& sceneGraph() noexcept { return *_sceneGraph; }
    inline const SceneGraph& sceneGraph() const noexcept { return *_sceneGraph; }

    void registerTask(Task& taskItem, bool start = true, TaskPriority priority = TaskPriority::DONT_CARE);
    void clearTasks();
    void removeTask(Task& task);
    inline void addSceneGraphToLoad(XML::SceneNode& rootNode) { _xmlSceneGraphRootNode = rootNode; }

    void addMusic(MusicType type, const Str64& name, const Str256& srcFile);

    SceneGraphNode* addSky(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName = "");
    SceneGraphNode* addInfPlane(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName = "");
    void addWater(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName = "");
    void addTerrain(SceneGraphNode& parentNode, boost::property_tree::ptree pt, const Str64& nodeName = "");

    /// Object picking
    inline Selections getCurrentSelection(PlayerIndex index = 0) const {
        const auto it = _currentSelection.find(index);
        if (it != eastl::cend(_currentSelection)) {
            return it->second;
        }

        return {};
    }

    bool findSelection(PlayerIndex idx, bool clearOld);
    void beginDragSelection(PlayerIndex idx, vec2<I32> mousePos);
    void endDragSelection(PlayerIndex idx, bool clearSelection);

    SceneGraphNode* addParticleEmitter(const Str64& name,
                                       std::shared_ptr<ParticleData> data,
                                       SceneGraphNode& parentNode);

    inline AI::AIManager& aiManager() noexcept { return *_aiManager; }
    inline const AI::AIManager& aiManager() const noexcept { return *_aiManager; }

    inline ResourceCache* resourceCache() noexcept { return _resCache; }
    inline const ResourceCache* resourceCache() const noexcept { return _resCache; }

    Camera* playerCamera() const;
    Camera* playerCamera(U8 index) const;

    inline LightPool& lightPool() noexcept { return *_lightPool; }
    inline const LightPool& lightPool() const noexcept { return *_lightPool; }

    // can save at any time, I guess?
    virtual bool saveXML(DELEGATE<void, const char*> msgCallback, DELEGATE<void, bool> finishCallback) const;


    void initDayNightCycle(Sky& skyInstance, DirectionalLightComponent& sunLight);

    // negative values should work
    void setDayNightCycleTimeFactor(F32 factor);
    F32 getDayNightCycleTimeFactor() const noexcept;

    void setTimeOfDay(const SimpleTime& time) noexcept;
    const SimpleTime& getTimeOfDay() const noexcept;

    SunDetails getCurrentSunDetails() const noexcept;
    Sky::Atmosphere getCurrentAtmosphere() const noexcept;
    void setCurrentAtmosphere(const Sky::Atmosphere& atmosphere) noexcept;

    PROPERTY_RW(bool, dayNightCycleEnabled, true);

   protected:
    struct DayNightData
    {
        Sky* _skyInstance = nullptr;
        DirectionalLightComponent* _dirLight = nullptr;
        F32 _speedFactor = 1.0f;
        F32 _timeAccumulator = 0.0f;
        SimpleTime _time = { 14u, 30u };
        bool _resetTime = true;
    } _dayNightData = {};

    virtual void rebuildShaders();
    virtual void onSetActive();
    virtual void onRemoveActive();
    // returns the first available action ID
    virtual U16 registerInputActions();
    virtual void loadKeyBindings();

    void onNodeDestroy(SceneGraphNode& node);
    void findHoverTarget(PlayerIndex idx, const vec2<I32>& aimPos);
    void clearHoverTarget(PlayerIndex idx);

    bool checkCameraUnderwater(PlayerIndex idx) const;
    bool checkCameraUnderwater(const Camera& camera) const;
    void toggleFlashlight(PlayerIndex idx);

    SceneNode_ptr createNode(SceneNodeType type, const ResourceDescriptor& descriptor);

    virtual bool save(ByteBuffer& outputBuffer) const;
    virtual bool load(ByteBuffer& inputBuffer);

    virtual bool frameStarted();
    virtual bool frameEnded();

    virtual void loadDefaultCamera();

    virtual bool loadXML(const Str128& name);

    virtual bool load(const Str128& name);
    void loadAsset(Task* parentTask, const XML::SceneNode& sceneNode, SceneGraphNode* parent);
    virtual bool unload();
    virtual void postLoad();
    // gets called on the main thread when the scene finishes loading (e.g. used by the GUI system)
    // We may be rendering in a different viewport (splash screen, loading screen, etc) but we need our GUI stuff
    // to be ready for our game viewport, so we pass it here to make sure we are using the proper one
    virtual void postLoadMainThread(const Rect<U16>& targetRenderViewport);
    /// Check if Scene::load() was called
    bool checkLoadFlag() const noexcept { return _loadComplete; }
    /// Unload scenegraph
    void clearObjects();
    /**End loading and unloading logic*/
    /// returns true if the camera was moved/rotated/etc
    bool updateCameraControls(PlayerIndex idx);
    /// Draw debug entities
    virtual void debugDraw(const Camera& activeCamera, RenderStagePass stagePass, GFX::CommandBuffer& bufferInOut);
    /// Draw custom ui elements
    virtual void drawCustomUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);

    //Return true if input was consumed
    virtual bool mouseMoved(const Input::MouseMoveEvent& arg);
    
    U8 getSceneIndexForPlayer(PlayerIndex idx) const;
    Player* getPlayerForIndex(PlayerIndex idx) const;

    U8 getPlayerIndexForDevice(U8 deviceIndex) const;

    void addPlayerInternal(bool queue);
    void removePlayerInternal(PlayerIndex idx);
    void onPlayerAdd(const Player_ptr& player);
    void onPlayerRemove(const Player_ptr& player);

    /// simple function to load the scene elements.
    inline bool SCENE_LOAD(const Str128& name) {
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
    void setSelected(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgn, bool recursive);

    bool lockCameraToPlayerMouse(PlayerIndex index, bool lockState);

    const char* getResourceTypeName() const noexcept override { return "Scene"; }


    void updateSelectionData(PlayerIndex idx, DragSelectData& data, bool remaped);

   protected:
       /// Global info
       SceneManager& _parent;

       ResourceCache* _resCache = nullptr;
       SceneGraph*    _sceneGraph = nullptr;
       AI::AIManager* _aiManager = nullptr;
       SceneGUIElements* _GUI = nullptr;

       vectorEASTL<Player*> _scenePlayers;
       U64 _sceneTimerUS = 0ULL;
       vectorEASTL<D64> _taskTimers;
       vectorEASTL<D64> _guiTimersMS;
       /// Datablocks for models,vegetation,terrains,tasks etc
       std::atomic_uint _loadingTasks;
       XML::SceneNode _xmlSceneGraphRootNode;

       F32 _LRSpeedFactor = 1.0f;
       /// Current selection
       hashMap<PlayerIndex, Selections> _currentSelection;
       hashMap<PlayerIndex, I64> _currentHoverTarget;
       hashMap<PlayerIndex, DragSelectData> _dragSelectData;

       hashMap<PlayerIndex, SceneGraphNode*> _flashLight;
       hashMap<PlayerIndex, U32> _cameraUpdateListeners;
       /// Scene::load must be called by every scene. Add a load flag to make sure!
       bool _loadComplete = false;
       /// Schedule a scene graph parse with the physics engine to recreate/recheck
       /// the collision meshes used by each node
       bool _cookCollisionMeshesScheduled = false;

   private:
       SharedMutex _tasksMutex;
       vectorEASTL<Task*> _tasks;
       /// Contains all game related info for the scene (wind speed, visibility ranges, etc)
       SceneState* _sceneState = nullptr;
       vectorEASTL<SGNRayResult> _sceneSelectionCandidates;

   protected:
       LightPool* _lightPool = nullptr;
       SceneInput* _input = nullptr;
       PhysicsSceneInterface* _pxScene = nullptr;
       SceneEnvironmentProbePool* _envProbePool = nullptr;

       IMPrimitive* _linesPrimitive = nullptr;
       vectorEASTL<IMPrimitive*> _octreePrimitives;
       vectorEASTL<BoundingBox> _octreeBoundingBoxes;

       //mutable Mutex _perFrameArenaMutex;
       //mutable MyArena<Config::REQUIRED_RAM_SIZE / 3> _perFrameArena;
};

namespace Attorney {
class SceneManager {
   private:
    static bool updateCameraControls(Scene& scene, PlayerIndex idx) {
        return scene.updateCameraControls(idx);
    }

    static bool checkLoadFlag(const Scene& scene) noexcept {
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

    static void drawCustomUI(Scene& scene, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
        scene.drawCustomUI(targetViewport, bufferInOut);
    }

    static bool frameStarted(Scene& scene) { return scene.frameStarted(); }
    static bool frameEnded(Scene& scene) { return scene.frameEnded(); }

    static bool loadXML(Scene& scene, const Str128& name) {
        return scene.loadXML(name);
    }
    static bool load(Scene& scene, const Str128& name) {
        return scene.load(name);
    }

    static bool unload(Scene& scene) { 
        return scene.unload();
    }

    static void postLoad(Scene& scene) {
        scene.postLoad();
    }

    static void postLoadMainThread(Scene& scene, const Rect<U16>& targetRenderViewport) {
        scene.postLoadMainThread(targetRenderViewport);
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

    static SceneGUIElements* gui(Scene& scene) noexcept {
        return scene._GUI;
    }

    static void resetSelection(Scene& scene, PlayerIndex idx) {
        scene.resetSelection(idx);
    }

    static void setSelected(Scene& scene, PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgns, bool recursive) {
        scene.setSelected(idx, sgns, recursive);
    }

    static void clearHoverTarget(Scene& scene, const Input::MouseMoveEvent& arg) {
        scene.clearHoverTarget(scene.input().getPlayerIndexForDevice(arg._deviceIndex));
    }

    static SceneNode_ptr createNode(Scene& scene, SceneNodeType type, const ResourceDescriptor& descriptor) {
        return scene.createNode(type, descriptor);
    }

    friend class Divide::SceneManager;
};

class SceneRenderPass {
 private:
    static SceneEnvironmentProbePool* getEnvProbes(Scene& scene) noexcept {
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
    static SceneGUIElements* guiElements(Scene& scene) noexcept {
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
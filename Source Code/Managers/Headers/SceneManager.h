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
#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Scenes/Headers/Scene.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

namespace Divide {

class LoadSave {
public:
    static bool loadScene(Scene& activeScene);
    static bool saveScene(const Scene& activeScene, bool toCache, DELEGATE<void, std::string_view> msgCallback, DELEGATE<void, bool> finishCallback);

    static bool saveNodeToXML(const Scene& activeScene, const SceneGraphNode& node);
    static bool loadNodeFromXML(const Scene& activeScene, SceneGraphNode& node);
};

enum class RenderStage : U8;
namespace Attorney {
    class SceneManagerScene;
    class SceneManagerEditor;
    class SceneManagerKernel;
    class SceneManagerScenePool;
    class SceneManagerRenderPass;
    class SceneManagerCameraAccessor;
};
namespace AI {
    namespace Navigation {
        class DivideRecats;
    };
};

class Editor;
class ScenePool;
class UnitComponent;
class SceneShaderData;
class ShaderComputeQueue;
class SolutionExplorerWindow;
class GUIConsoleCommandParser;
FWD_DECLARE_MANAGED_CLASS(Player);

class SceneManager : public FrameListener,
                     public Input::InputAggregatorInterface,
                     public KernelComponent {

    friend class Attorney::SceneManagerScene;
    friend class Attorney::SceneManagerEditor;
    friend class Attorney::SceneManagerKernel;
    friend class Attorney::SceneManagerScenePool;
    friend class Attorney::SceneManagerRenderPass;
    friend class Attorney::SceneManagerCameraAccessor;

public:
    using PlayerList = std::array<UnitComponent*, Config::MAX_LOCAL_PLAYER_COUNT>;

public:
    static bool onStartup();
    static bool onShutdown();

    explicit SceneManager(Kernel& parentKernel);
    ~SceneManager();

    void idle();

    vectorEASTL<Str128> sceneNameList(bool sorted = true) const;

    Scene& getActiveScene();
    const Scene& getActiveScene() const;

    void setActiveScene(Scene* const scene);

    bool init(PlatformContext& platformContext, ResourceCache* cache);
    void destroy();

    inline U8 getActivePlayerCount() const noexcept { return _activePlayerCount; }

    inline void addSelectionCallback(const DELEGATE<void, U8, const vectorEASTL<SceneGraphNode*>&>& selectionCallback) {
        _selectionChangeCallbacks.push_back(selectionCallback);
    }
    void resetSelection(PlayerIndex idx);
    void setSelected(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgns, bool recursive);
    void onNodeDestroy(SceneGraphNode& node);
    // cull the scenegraph against the current view frustum
    const VisibleNodeList& cullSceneGraph(RenderStage stage, const Camera& camera, I32 maxLoD, const vec3<F32>& minExtents, I64* ignoredGUIDS, size_t ignoredGUIDSCount);
    // get the full list of reflective nodes
    void getSortedReflectiveNodes(const Camera& camera, RenderStage stage, bool inView, VisibleNodeList& nodesOut) const;
    // get the full list of refractive nodes
    void getSortedRefractiveNodes(const Camera& camera, RenderStage stage, bool inView, VisibleNodeList& nodesOut) const;

    void onLostFocus();
    void onGainFocus();

    /// Check if the scene was loaded properly
    inline bool checkLoadFlag() const {
        return Attorney::SceneManager::checkLoadFlag(getActiveScene());
    }
    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTimeUS);

    /// Gather input events and process them in the current scene
    inline void processInput(PlayerIndex idx, const U64 deltaTimeUS) {
        OPTICK_EVENT();

        getActiveScene().processInput(idx, deltaTimeUS);
        Attorney::SceneManager::updateCameraControls(getActiveScene(), idx);
    }

    inline void processTasks(const U64 deltaTimeUS) {
        OPTICK_EVENT();

        getActiveScene().processTasks(deltaTimeUS);
    }

    inline void processGUI(const U64 deltaTimeUS) {
        OPTICK_EVENT();

        getActiveScene().processGUI(deltaTimeUS);
    }

    inline void onStartUpdateLoop(const U8 loopNumber) {
        getActiveScene().onStartUpdateLoop(loopNumber);
    }

    void onSizeChange(const SizeChangeParams& params);

    inline U8 playerPass() const noexcept { return _currentPlayerPass; }

    template <typename T, class Factory>
    bool register_new_ptr(Factory& factory, BOOST_DEDUCED_TYPENAME Factory::id_param_type id) {
        return factory.register_creator(id, new_ptr<T>());
    }

    bool saveActiveScene(bool toCache, bool deferred, DELEGATE<void, std::string_view> msgCallback = {}, DELEGATE<void, bool> finishCallback = {});

    AI::Navigation::DivideRecast* recast() const noexcept { return _recast.get(); }

public:  /// Input
  /// Key pressed: return true if input was consumed
    bool onKeyDown(const Input::KeyEvent& key) override;
    /// Key released: return true if input was consumed
    bool onKeyUp(const Input::KeyEvent& key) override;
    /// Joystick axis change: return true if input was consumed
    bool joystickAxisMoved(const Input::JoystickEvent& arg) override;
    /// Joystick direction change: return true if input was consumed
    bool joystickPovMoved(const Input::JoystickEvent& arg) override;
    /// Joystick button pressed: return true if input was consumed
    bool joystickButtonPressed(const Input::JoystickEvent& arg) override;
    /// Joystick button released: return true if input was consumed
    bool joystickButtonReleased(const Input::JoystickEvent& arg) override;
    bool joystickBallMoved(const Input::JoystickEvent& arg) override;
    // return true if input was consumed
    bool joystickAddRemove(const Input::JoystickEvent& arg) override;
    bool joystickRemap(const Input::JoystickEvent &arg) override;
    /// Mouse moved: return true if input was consumed
    bool mouseMoved(const Input::MouseMoveEvent& arg) override;
    /// Mouse button pressed: return true if input was consumed
    bool mouseButtonPressed(const Input::MouseButtonEvent& arg) override;
    /// Mouse button released: return true if input was consumed
    bool mouseButtonReleased(const Input::MouseButtonEvent& arg) override;

    bool onUTF8(const Input::UTF8Event& arg) override;

    bool switchScene(const Str128& name,
                     bool unloadPrevious,
                     const Rect<U16>& targetRenderViewport,
                     bool threaded = true);


    /// Called if a mouse move event was captured by a different system (editor, gui, etc).
    /// Used to cancel scene specific mouse move tracking
    void mouseMovedExternally(const Input::MouseMoveEvent& arg);

    PROPERTY_RW(bool, wantsMouse, false);
    PROPERTY_RW(bool, wantsKeyboard, false);

// networking
protected:
    bool networkUpdate(U32 frameCount);

protected:
    void initPostLoadState();
    Scene* load(const Str128& name);
    bool   unloadScene(Scene* scene);

    // Add a new player to the simulation
    void addPlayerInternal(Scene& parentScene, SceneGraphNode* playerNode);
    // Removes the specified player from the active simulation
    // Returns true if the player was previously registered
    // On success, player pointer will be reset
    void removePlayerInternal(Scene& parentScene, SceneGraphNode* playerNode);

    // Add a new player to the simulation
    void addPlayer(Scene& parentScene, SceneGraphNode* playerNode, bool queue);
    // Removes the specified player from the active simulation
    // Returns true if the player was previously registered
    // On success, player pointer will be reset
    void removePlayer(Scene& parentScene, SceneGraphNode* playerNode, bool queue);
    vectorEASTL<SceneGraphNode*> getNodesInScreenRect(const Rect<I32>& screenRect, const Camera& camera, const Rect<I32>& viewport) const;

protected:
    bool frameStarted(const FrameEvent& evt) override;
    bool frameEnded(const FrameEvent& evt) override;
    void preRender(const RenderStagePass& stagePass, const Camera& camera, const Texture_ptr& hizColourTexture, GFX::CommandBuffer& bufferInOut);
    void postRender(const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut);
    void preRenderAllPasses(const Camera& playerCamera);
    void postRenderAllPasses(const Camera& playerCamera);
    void drawCustomUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);
    void debugDraw(const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut);
    void prepareLightData(RenderStage stage, const vec3<F32>& cameraPos, const mat4<F32>& viewMatrix);
    void generateShadowMaps(GFX::CommandBuffer& bufferInOut);

    Camera* playerCamera() const;
    Camera* playerCamera(PlayerIndex idx) const;
    void currentPlayerPass(PlayerIndex idx);
    void moveCameraToNode(const SceneGraphNode& targetNode) const;
    bool saveNode(const SceneGraphNode& targetNode) const;
    bool loadNode(SceneGraphNode& targetNode) const;
    SceneNode_ptr createNode(SceneNodeType type, const ResourceDescriptor& descriptor);
    SceneEnvironmentProbePool* getEnvProbes() const;

private:
    bool _init = false;
    bool _processInput = false;
    /// Pointer to the hardware objects
    PlatformContext* _platformContext = nullptr;
    /// Pointer to the general purpose resource cache
    ResourceCache*  _resourceCache = nullptr;
    /// Pointer to the scene graph culler that's used to determine what nodes are
    /// visible in the current frame
    RenderPassCuller* _renderPassCuller = nullptr;

    Task* _saveTask = nullptr;
    PlayerIndex _currentPlayerPass = 0u;
    ScenePool* _scenePool = nullptr;
    SceneShaderData* _sceneData = nullptr;
    U64 _elapsedTime = 0ULL;
    U32 _elapsedTimeMS = 0u;
    U64 _saveTimer = 0ULL;

    std::array<Time::ProfileTimer*, to_base(RenderStage::COUNT)> _sceneGraphCullTimers = {};
    PlayerList _players = {};
    U8 _activePlayerCount = 0u;

    bool _playerQueueDirty = false;
    eastl::queue<std::pair<Scene*, SceneGraphNode*>>  _playerAddQueue;
    eastl::queue<std::pair<Scene*, SceneGraphNode*>>  _playerRemoveQueue;
    eastl::unique_ptr<AI::Navigation::DivideRecast> _recast = nullptr;

    vectorEASTL<DELEGATE<void, U8 /*player index*/, const vectorEASTL<SceneGraphNode*>& /*nodes*/> > _selectionChangeCallbacks;

    struct SwitchSceneTarget {
        Str128 _targetSceneName = "";
        Rect<U16> _targetViewRect = { 0, 0, 1, 1 };
        bool _unloadPreviousScene = true;
        bool _loadInSeparateThread = true;
        bool _isSet = false;
    } _sceneSwitchTarget;

};

namespace Attorney {
class SceneManagerScene {
private:
    static void addPlayer(Divide::SceneManager& manager, Scene& parentScene, SceneGraphNode* playerNode, bool queue) {
        manager.addPlayer(parentScene, playerNode, queue);
    }

    static void removePlayer(Divide::SceneManager& manager, Scene& parentScene, SceneGraphNode* playerNode, bool queue) {
        manager.removePlayer(parentScene, playerNode, queue);
    }

    static vectorEASTL<SceneGraphNode*> getNodesInScreenRect(const Divide::SceneManager& manager, const Rect<I32>& screenRect, const Camera& camera, const Rect<I32>& viewport) {
        return manager.getNodesInScreenRect(screenRect, camera, viewport);
    }

    friend class Divide::Scene;
};

class SceneManagerKernel {
   private:
    static void initPostLoadState(Divide::SceneManager& manager) {
        manager.initPostLoadState();
    }

    static void currentPlayerPass(Divide::SceneManager& manager, PlayerIndex idx) {
        manager.currentPlayerPass(idx);
    }

    static bool networkUpdate(Divide::SceneManager& manager, U32 frameCount) {
        return manager.networkUpdate(frameCount);
    }

    friend class Divide::Kernel;
};

class SceneManagerEditor {
  private:
   static SceneNode_ptr createNode(Divide::SceneManager& manager, SceneNodeType type, const ResourceDescriptor& descriptor) {
     return manager.createNode(type, descriptor);
   }

   static SceneEnvironmentProbePool* getEnvProbes(const Divide::SceneManager& manager) {
       return manager.getEnvProbes();
   }

   static bool saveNode(Divide::SceneManager& mgr, const SceneGraphNode& targetNode) {
       return mgr.saveNode(targetNode);
   }

   static bool loadNode(Divide::SceneManager& mgr, SceneGraphNode& targetNode) {
       return mgr.loadNode(targetNode);
   }

   friend class Divide::Editor;
};
class SceneManagerScenePool {
  private:
   static bool unloadScene(Divide::SceneManager& mgr, Scene* scene) {
       return mgr.unloadScene(scene);
   }

   friend class Divide::ScenePool;
};

class SceneManagerCameraAccessor {
  private:
    static Camera* playerCamera(Divide::SceneManager& mgr) {
        return mgr.playerCamera();
    }

    static Camera* playerCamera(Divide::SceneManager& mgr, PlayerIndex idx) {
        return mgr.playerCamera(idx);
    }

    static void moveCameraToNode(Divide::SceneManager& mgr, const SceneGraphNode& targetNode) {
        mgr.moveCameraToNode(targetNode);
    }


    friend class Divide::Scene;
    friend class Divide::Editor;
    friend class Divide::ShadowMap;
    friend class Divide::RenderPass;
    friend class Divide::SolutionExplorerWindow;
    friend class Divide::GUIConsoleCommandParser;
};

class SceneManagerRenderPass {
   private:
    static const VisibleNodeList& cullScene(Divide::SceneManager& mgr, RenderStage stage, const Camera& camera, I32 minLoD, const vec3<F32>& minExtents, I64* ignoredGUIDS, size_t ignoredGUIDSCount) {
        return mgr.cullSceneGraph(stage, camera, minLoD, minExtents, ignoredGUIDS, ignoredGUIDSCount);
    }

    static void prepareLightData(Divide::SceneManager& mgr, RenderStage stage, const vec3<F32>& camPosition, const mat4<F32>& viewMatrix) {
        mgr.prepareLightData(stage, camPosition, viewMatrix);
    }

    static void preRenderMainPass(Divide::SceneManager& mgr, const RenderStagePass& stagePass, const Camera& camera, const Texture_ptr& hizColourTexture, GFX::CommandBuffer& bufferInOut) {
        mgr.preRender(stagePass, camera, hizColourTexture, bufferInOut);
    }

    static void postRender(Divide::SceneManager& mgr, const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
        mgr.postRender(stagePass, camera, bufferInOut);
    }

    static void preRenderAllPasses(Divide::SceneManager& mgr, const Camera& playerCamera) {
        mgr.preRenderAllPasses(playerCamera);
    }

    static void postRenderAllPasses(Divide::SceneManager& mgr, const Camera& playerCamera) {
        mgr.postRenderAllPasses(playerCamera);
    }

    static void debugDraw(Divide::SceneManager& mgr, const RenderStagePass& stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
        mgr.debugDraw(stagePass, camera, bufferInOut);
    }

    static void drawCustomUI(Divide::SceneManager& mgr, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
        mgr.drawCustomUI(targetViewport, bufferInOut);
    }

    static void generateShadowMaps(Divide::SceneManager& mgr, GFX::CommandBuffer& bufferInOut) {
        mgr.generateShadowMaps(bufferInOut);
    }

    static const Camera& playerCamera(const Divide::SceneManager& mgr) {
        return *mgr.playerCamera();
    }

    static const SceneStatePerPlayer& playerState(const Divide::SceneManager& mgr) {
        return mgr.getActiveScene().state().playerState();
    }

    friend class Divide::RenderPass;
    friend class Divide::RenderPassManager;
};

};  // namespace Attorney

};  // namespace Divide

#endif

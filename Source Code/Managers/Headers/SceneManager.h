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
    static bool saveScene(const Scene& activeScene, bool toCache);
};

enum class RenderStage : U8;
namespace Attorney {
    class SceneManagerScene;
    class SceneManagerKernel;
    class SceneManagerScenePool;
    class SceneManagerRenderPass;
    class SceneManagerCameraAccessor;
};

class Editor;
class ScenePool;
class SceneShaderData;
class ShaderComputeQueue;
class SolutionExplorerWindow;
class GUIConsoleCommandParser;
FWD_DECLARE_MANAGED_CLASS(Player);

class SceneManager : public FrameListener,
                     public Input::InputAggregatorInterface,
                     public KernelComponent {

    friend class Attorney::SceneManagerScene;
    friend class Attorney::SceneManagerKernel;
    friend class Attorney::SceneManagerScenePool;
    friend class Attorney::SceneManagerRenderPass;
    friend class Attorney::SceneManagerCameraAccessor;

public:
    using PlayerList = std::array<SceneGraphNode*, Config::MAX_LOCAL_PLAYER_COUNT>;

public:
    static bool onStartup();
    static bool onShutdown();

    explicit SceneManager(Kernel& parentKernel);
    ~SceneManager();

    void idle();

    vector<Str128> sceneNameList(bool sorted = true) const;

    Scene& getActiveScene();
    const Scene& getActiveScene() const;

    void setActiveScene(Scene* const scene);

    bool init(PlatformContext& platformContext, ResourceCache& cache);
    void destroy();

    inline U8 getActivePlayerCount() const { return _activePlayerCount; }

    inline void addSelectionCallback(const DELEGATE_CBK<void, U8, const vectorEASTL<SceneGraphNode*>&>& selectionCallback) {
        _selectionChangeCallbacks.push_back(selectionCallback);
    }
    void resetSelection(PlayerIndex idx);
    void setSelected(PlayerIndex idx, const vectorEASTL<SceneGraphNode*>& sgns);

    // cull the scenegraph against the current view frustum
    const VisibleNodeList& cullSceneGraph(RenderStage stage, const Camera& camera, I32 maxLoD, const vec3<F32>& minExtents);
    // get the full list of reflective nodes
    VisibleNodeList getSortedReflectiveNodes(const Camera& camera, RenderStage stage, bool inView) const;
    // get the full list of refractive nodes
    VisibleNodeList getSortedRefractiveNodes(const Camera& camera, RenderStage stage, bool inView) const;

    void onLostFocus();
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

    inline U8 playerPass() const { return _currentPlayerPass; }

    template <typename T, class Factory>
    bool register_new_ptr(Factory& factory, BOOST_DEDUCED_TYPENAME Factory::id_param_type id) {
        return factory.register_creator(id, new_ptr<T>());
    }

    bool saveActiveScene(bool toCache, bool deferred = true);

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
    vectorEASTL<SceneGraphNode*> getNodesInScreenRect(const Rect<I32>& screenRect, const Camera& camera, const Rect<I32>& viewport, bool editorRunning) const;

protected:
    bool frameStarted(const FrameEvent& evt)  noexcept override;
    bool frameEnded(const FrameEvent& evt)  noexcept override;
    void preRender(RenderStagePass stagePass, const Camera& camera, const Texture_ptr& hizColourTexture, GFX::CommandBuffer& bufferInOut);
    void postRender(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut);
    void preRenderAllPasses(const Camera& playerCamera);
    void postRenderAllPasses(const Camera& playerCamera);
    void drawCustomUI(const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut);
    void debugDraw(RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut);
    void prepareLightData(RenderStage stage, const Camera& camera);
    void generateShadowMaps(GFX::CommandBuffer& bufferInOut);

    Camera* playerCamera() const;
    Camera* playerCamera(PlayerIndex idx) const;
    void currentPlayerPass(PlayerIndex idx);

private:
    bool _init;
    bool _processInput;
    /// Pointer to the hardware objects
    PlatformContext* _platformContext;
    /// Pointer to the general purpose resource cache
    ResourceCache*  _resourceCache;
    /// Pointer to the scene graph culler that's used to determine what nodes are
    /// visible in the current frame
    RenderPassCuller* _renderPassCuller;

    Task* _saveTask = nullptr;
    PlayerIndex _currentPlayerPass;
    ScenePool* _scenePool;
    SceneShaderData* _sceneData;
    U64 _elapsedTime;
    U32 _elapsedTimeMS;
    U64 _saveTimer;

    std::array<Time::ProfileTimer*, to_base(RenderStage::COUNT)> _sceneGraphCullTimers;
    PlayerList _players;
    U8 _activePlayerCount;

    bool _platerQueueDirty = false;
    std::queue<std::pair<Scene*, SceneGraphNode*>>  _playerAddQueue;
    std::queue<std::pair<Scene*, SceneGraphNode*>>  _playerRemoveQueue;

    vector<DELEGATE_CBK<void, U8 /*player index*/, const vectorEASTL<SceneGraphNode*>& /*nodes*/> > _selectionChangeCallbacks;

    struct SwitchSceneTarget {
        SwitchSceneTarget() noexcept
            : _targetSceneName(""),
            _targetViewRect(0, 0, 1, 1),
            _unloadPreviousScene(true),
            _loadInSeparateThread(true),
            _isSet(false)
        {
        }

        inline void reset() {
            _targetSceneName.clear();
            _targetViewRect.set(0, 0, 1, 1);
            _unloadPreviousScene = true;
            _loadInSeparateThread = true;
            _isSet = false;
        }

        inline bool isSet() const {
            return _isSet;
        }

        inline void set(const Str128& targetSceneName,
            const Rect<U16>& targetViewRect,
            bool unloadPreviousScene,
            bool loadInSeparateThread)
        {
            _targetSceneName = targetSceneName;
            _targetViewRect.set(targetViewRect);
            _unloadPreviousScene = unloadPreviousScene;
            _loadInSeparateThread = loadInSeparateThread;
            _isSet = true;
        }

        inline const Str128& targetSceneName() const {
            return _targetSceneName;
        }

        inline const Rect<U16>& targetViewRect() const {
            return _targetViewRect;
        }

        inline bool unloadPreviousScene() const {
            return _unloadPreviousScene;
        }

        inline bool loadInSeparateThread() const {
            return _loadInSeparateThread;
        }

    private:
        Str128 _targetSceneName;
        Rect<U16> _targetViewRect;
        bool _unloadPreviousScene;
        bool _loadInSeparateThread;
        bool _isSet;
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

    static vectorEASTL<SceneGraphNode*> getNodesInScreenRect(const Divide::SceneManager& manager, const Rect<I32>& screenRect, const Camera& camera, const Rect<I32>& viewport, bool editorRunning) {
        return manager.getNodesInScreenRect(screenRect, camera, viewport, editorRunning);
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

    friend class Divide::Scene;
    friend class Divide::Editor;
    friend class Divide::ShadowMap;
    friend class Divide::RenderPass;
    friend class Divide::SolutionExplorerWindow;
    friend class Divide::GUIConsoleCommandParser;
};

class SceneManagerRenderPass {
   private:
    static const VisibleNodeList& cullScene(Divide::SceneManager& mgr, RenderStage stage, const Camera& camera, I32 minLoD, const vec3<F32>& minExtents) {
        return mgr.cullSceneGraph(stage, camera, minLoD, minExtents);
    }

    static void prepareLightData(Divide::SceneManager& mgr,
                                 RenderStage stage,
                                 const Camera& camera) {
        mgr.prepareLightData(stage, camera);
    }

    static void preRenderMainPass(Divide::SceneManager& mgr, RenderStagePass stagePass, const Camera& camera, const Texture_ptr& hizColourTexture, GFX::CommandBuffer& bufferInOut) {
        mgr.preRender(stagePass, camera, hizColourTexture, bufferInOut);
    }

    static void postRender(Divide::SceneManager& mgr, RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
        mgr.postRender(stagePass, camera, bufferInOut);
    }

    static void preRenderAllPasses(Divide::SceneManager& mgr, const Camera& playerCamera) {
        mgr.preRenderAllPasses(playerCamera);
    }

    static void postRenderAllPasses(Divide::SceneManager& mgr, const Camera& playerCamera) {
        mgr.postRenderAllPasses(playerCamera);
    }

    static void debugDraw(Divide::SceneManager& mgr, RenderStagePass stagePass, const Camera& camera, GFX::CommandBuffer& bufferInOut) {
        mgr.debugDraw(stagePass, camera, bufferInOut);
    }

    static void drawCustomUI(Divide::SceneManager& mgr, const Rect<I32>& targetViewport, GFX::CommandBuffer& bufferInOut) {
        mgr.drawCustomUI(targetViewport, bufferInOut);
    }

    static void generateShadowMaps(Divide::SceneManager& mgr, GFX::CommandBuffer& bufferInOut) {
        mgr.generateShadowMaps(bufferInOut);
    }

    static const Camera& playerCamera(Divide::SceneManager& mgr) {
        return *mgr.playerCamera();
    }

    friend class Divide::RenderPass;
    friend class Divide::RenderPassManager;
};

};  // namespace Attorney

};  // namespace Divide

#endif

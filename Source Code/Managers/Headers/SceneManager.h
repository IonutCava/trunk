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

#ifndef _SCENE_MANAGER_H
#define _SCENE_MANAGER_H

#include "Scenes/Headers/Scene.h"
#include "Rendering/RenderPass/Headers/RenderPassCuller.h"

namespace Divide {

class LoadSave {
public:
    static bool loadScene(Scene& activeScene);
    static bool saveScene(const Scene& activeScene);
};

enum class RenderStage : U32;
namespace Attorney {
    class SceneManagerKernel;
    class SceneManagerRenderPass;
};

class ScenePool;
class SceneShaderData;
DEFINE_SINGLETON(SceneManager, FrameListener, Input::InputAggregatorInterface)
    friend class Attorney::SceneManagerKernel;
    friend class Attorney::SceneManagerRenderPass;
  public:
    static bool initStaticData();

    void idle();

    vectorImpl<stringImpl> sceneNameList(bool sorted = true) const;

    Scene& getActiveScene();
    const Scene& getActiveScene() const;

    void setActiveScene(Scene* const scene);

    bool init(GUI* const gui);

    /*Base Scene Operations*/
    Renderer& getRenderer() const;
    void setRenderer(RendererType rendererType);

    // generate a list of nodes to render
    void updateVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass = 0);
    void renderVisibleNodes(RenderStage stage, bool refreshNodeData, U32 pass = 0);
    // cull the scenegraph against the current view frustum
    const RenderPassCuller::VisibleNodeList& cullSceneGraph(RenderStage stage);
    // get the full list of reflective nodes
    const RenderPassCuller::VisibleNodeList& getSortedReflectiveNodes();

    void onLostFocus();
    /// Check if the scene was loaded properly
    inline bool checkLoadFlag() const {
        return Attorney::SceneManager::checkLoadFlag(getActiveScene());
    }
    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTime);

    /// Gather input events and process them in the current scene
    inline void processInput(const U64 deltaTime) {
        getActiveScene().processInput(deltaTime);
        Attorney::SceneManager::updateCameraControls(getActiveScene());
    }

    inline void processTasks(const U64 deltaTime) {
        getActiveScene().processTasks(deltaTime);
    }
    inline void processGUI(const U64 deltaTime) {
        getActiveScene().processGUI(deltaTime);
    }

    RenderPassCuller::VisibleNodeList& getVisibleNodesCache(RenderStage stage);

    template <typename T, class Factory>
    bool register_new_ptr(Factory& factory, BOOST_DEDUCED_TYPENAME Factory::id_param_type id) { 
        return factory.register_creator(id, new_ptr<T>());
    }

  public:  /// Input
    /// Key pressed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystic axis change
    bool joystickAxisMoved(const Input::JoystickEvent& arg, I8 axis);
    /// Joystick direction change
    bool joystickPovMoved(const Input::JoystickEvent& arg, I8 pov);
    /// Joystick button pressed
    bool joystickButtonPressed(const Input::JoystickEvent& arg,
                               Input::JoystickButton button);
    /// Joystick button released
    bool joystickButtonReleased(const Input::JoystickEvent& arg,
                                Input::JoystickButton button);
    bool joystickSliderMoved(const Input::JoystickEvent& arg, I8 index);
    bool joystickVector3DMoved(const Input::JoystickEvent& arg, I8 index);
    /// Mouse moved
    bool mouseMoved(const Input::MouseEvent& arg);
    /// Mouse button pressed
    bool mouseButtonPressed(const Input::MouseEvent& arg,
                            Input::MouseButton button);
    /// Mouse button released
    bool mouseButtonReleased(const Input::MouseEvent& arg,
                             Input::MouseButton button);

    bool switchScene(const stringImpl& name, bool unloadPrevious, bool threaded = true);

  protected:
    friend class ScenePool;
    void initPostLoadState();
    Scene* load(stringImpl name);
    bool   unloadScene(Scene* scene);

  protected:
    bool frameStarted(const FrameEvent& evt) override;
    bool frameEnded(const FrameEvent& evt) override;
    void onCameraUpdate(Camera& camera);
    void preRender();
    bool generateShadowMaps();

  private:
    SceneManager();
    ~SceneManager();

  private:
    bool _init;
    bool _processInput;
    /// Pointer to the GUI interface
    GUI* _GUI;
    /// Pointer to the scene graph culler that's used to determine what nodes are
    /// visible in the current frame
    RenderPassCuller* _renderPassCuller;
    /// Pointer to the render pass manager
    RenderPassManager* _renderPassManager;
    ScenePool* _scenePool;
    SceneShaderData* _sceneData;
    U64 _elapsedTime;
    U32 _elapsedTimeMS;
    U64 _saveTimer;
    Renderer* _renderer;
    Material_ptr _defaultMaterial;
    RenderPassCuller::VisibleNodeList _reflectiveNodesCache;
    Time::ProfileTimer& _sceneGraphCullTimer;

    struct SwitchSceneTarget {
        SwitchSceneTarget()
            : _targetSceneName(""),
              _unloadPreviousScene(true),
              _loadInSeparateThread(true),
              _isSet(false)
        {
        }

        inline void reset() {
            _targetSceneName.clear();
            _unloadPreviousScene = true;
            _loadInSeparateThread = true;
            _isSet = false;
        }

        inline bool isSet() const {
            return _isSet;
        }

        inline void set(const stringImpl& targetSceneName,
                        bool unloadPreviousScene,
                        bool loadInSeparateThread) {
            _targetSceneName = targetSceneName;
            _unloadPreviousScene = unloadPreviousScene;
            _loadInSeparateThread = loadInSeparateThread;
            _isSet = true;
        }

        inline const stringImpl& targetSceneName() const {
            return _targetSceneName;
        }

        inline bool unloadPreviousScene() const {
            return _unloadPreviousScene;
        }

        inline bool loadInSeparateThread() const {
            return _loadInSeparateThread;
        }

     private:
        stringImpl _targetSceneName;
        bool _unloadPreviousScene;
        bool _loadInSeparateThread;
        bool _isSet;
    } _sceneSwitchTarget;

END_SINGLETON

namespace Attorney {
class SceneManagerKernel {
   private:
    static void initPostLoadState() {
        Divide::SceneManager::instance().initPostLoadState();
    }

    static void onCameraUpdate(Camera& camera) {
        Divide::SceneManager::instance().onCameraUpdate(camera);
    }

    friend class Divide::Kernel;
};

class SceneManagerRenderPass {
   private:
    static void preRender(Divide::SceneManager& mgr) {
        mgr.preRender();
    }

    static bool generateShadowMaps(Divide::SceneManager& mgr) {
        return mgr.generateShadowMaps();
    }

    friend class Divide::RenderPass;
};

};  // namespace Attorney

};  // namespace Divide

#endif

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

struct SceneShaderData {
    SceneShaderData()
    {
    }

    vec4<F32> _fogDetails;
    vec4<F32> _windDetails;
    vec4<F32> _shadowingSettings;
    vec4<F32> _otherData;
    vec4<F32> _otherData2;
    vec4<U32> _lightCountPerType;
    //U32       _lightCountPerType[to_const_uint(LightType::COUNT)];

    inline void fogDetails(F32 colorR, F32 colorG, F32 colorB, F32 density) {
        _fogDetails.set(colorR, colorG, colorB, density);
    }

    inline void fogDensity(F32 density) {
        _fogDetails.w = density;
    }

    inline void shadowingSettings(F32 lightBleedBias, F32 minShadowVariance, F32 shadowFadeDist, F32 shadowMaxDist) {
        _shadowingSettings.set(lightBleedBias, minShadowVariance, shadowFadeDist, shadowMaxDist);
    }

    inline void windDetails(F32 directionX, F32 directionY, F32 directionZ, F32 speed) {
        _windDetails.set(directionX, directionY, directionZ, speed);
    }

    inline void elapsedTime(U32 timeMS) {
        _otherData.x = to_float(timeMS);
    }

    inline void enableDebugRender(bool state) {
        _otherData.y = state ? 1.0f : 0.0f;
    }

    inline void toggleShadowMapping(bool state) {
        _otherData.z = state ? 1.0f : 0.0f;
    }

    inline void setRendererFlag(U32 flag) {
        _otherData.w = to_float(flag);
    }

    inline void deltaTime(F32 deltaTimeSeconds) {
        _otherData2.x = deltaTimeSeconds;
    }

    inline void lightCount(LightType type, U32 lightCount) {
        _lightCountPerType[to_uint(type)] = lightCount;
    }
};

enum class RenderStage : U32;
namespace Attorney {
    class SceneManagerKernel;
};

DEFINE_SINGLETON_EXT2(SceneManager, FrameListener,
                      Input::InputAggregatorInterface)
    friend class Attorney::SceneManagerKernel;
  public:
    static bool initStaticData();
    /// Lookup the factory methods table and return the pointer to a newly
    /// constructed scene bound to that name
    Scene* createScene(const stringImpl& name);

    inline Scene& getActiveScene() { return *_activeScene; }
    inline void setActiveScene(Scene& scene) {
        _activeScene.reset(&scene);
    }

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

    inline void onLostFocus() { _activeScene->onLostFocus(); }
    inline void idle() { _activeScene->idle(); }
    bool unloadCurrentScene();
    bool load(const stringImpl& name);
    /// Check if the scene was loaded properly
    inline bool checkLoadFlag() const {
        return Attorney::SceneManager::checkLoadFlag(*_activeScene);
    }
    /// Create AI entities, teams, NPC's etc
    inline bool initializeAI(bool continueOnErrors) {
        return Attorney::SceneManager::initializeAI(*_activeScene,
                                                  continueOnErrors);
    }
    /// Update animations, network data, sounds, triggers etc.
    void updateSceneState(const U64 deltaTime);

    /// Gather input events and process them in the current scene
    inline void processInput(const U64 deltaTime) {
        _activeScene->processInput(deltaTime);
        Attorney::SceneManager::updateCameraControls(*_activeScene);
    }

    inline void processTasks(const U64 deltaTime) {
        _activeScene->processTasks(deltaTime);
    }
    inline void processGUI(const U64 deltaTime) {
        _activeScene->processGUI(deltaTime);
    }

    void enableFog(F32 density, const vec3<F32>& color);


    SceneShaderData& sceneData() {
        return _sceneData;
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

  protected:
    void initPostLoadState();

  protected:
    bool frameStarted(const FrameEvent& evt) override;
    bool frameEnded(const FrameEvent& evt) override;
    void onCameraUpdate(Camera& camera);

  private:
    SceneManager();
    ~SceneManager();

  private:
    typedef hashMapImpl<ULL, Scene*> SceneMap;
    bool _init;
    bool _processInput;
    /// Pointer to the currently active scene
    std::unique_ptr<Scene> _activeScene;
    /// Pointer to the GUI interface
    GUI* _GUI;
    /// Pointer to the scene graph culler that's used to determine what nodes are
    /// visible in the current frame
    RenderPassCuller* _renderPassCuller;
    /// Pointer to the render pass manager
    RenderPassManager* _renderPassManager;
    /// Generic scene data that doesn't change per shader
    std::unique_ptr<ShaderBuffer> _sceneShaderData;
    /// Scene pool
    SceneMap _sceneMap;
    Material* _defaultMaterial;
    SceneShaderData _sceneData;
    U64 _elapsedTime;
    U32 _elapsedTimeMS;
    U64 _saveTimer;
    std::unique_ptr<Renderer> _renderer;
    RenderPassCuller::VisibleNodeList _reflectiveNodesCache;
    Time::ProfileTimer& _sceneGraphCullTimer;
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
};  // namespace Attorney

/// Return a pointer to the currently active scene
inline Scene& GET_ACTIVE_SCENE() {
    return SceneManager::instance().getActiveScene();
}

/// Return a pointer to the currently active scene's scenegraph
inline SceneGraph& GET_ACTIVE_SCENEGRAPH() {
    return GET_ACTIVE_SCENE().getSceneGraph();
}

};  // namespace Divide

#endif

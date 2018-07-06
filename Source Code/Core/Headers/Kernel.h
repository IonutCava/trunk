/*
   Copyright (c) 2015 DIVIDE-Studio
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

#ifndef _CORE_KERNEL_H_
#define _CORE_KERNEL_H_

#include "Core/Headers/Application.h"
#include "Managers/Headers/CameraManager.h"
#include "Platform/Threading/Headers/Task.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace Divide {

class Scene;
class PXDevice;
class GFXDevice;
class SFXDevice;
class Application;
class LightManager;
class SceneManager;
class ShaderManager;
class InputInterface;
class SceneRenderState;
class FrameListenerManager;

enum class RenderStage : U32;

struct FrameEvent;
class GUI;

namespace Attorney {
    class KernelScene;
};

/// The kernel is the main interface to our engine components:
///-video
///-audio
///-physx
///-scene manager
///-etc
class Kernel : public Input::InputAggregatorInterface, private NonCopyable {
    friend class Attorney::KernelScene;

   public:
    Kernel(I32 argc, char** argv, Application& parentApp);
    ~Kernel();

    ErrorCode initialize(const stringImpl& entryPoint);

    void runLogicLoop();
    /// Our main application rendering loop.
    /// Call input requests, physics calculations, pre-rendering,
    /// rendering,post-rendering etc
    static void mainLoopApp();
    /// Called after a swap-buffer call and before a clear-buffer call.
    /// In a GPU-bound application, the CPU will wait on the GPU to finish
    /// processing the frame
    /// so this should keep it busy (old-GLUT heritage)
    static void idle();
    /// Update all engine components that depend on the current resolution
    static void updateResolutionCallback(I32 w, I32 h);

    GFXDevice& getGFXDevice() const { return _GFX; }
    SFXDevice& getSFXDevice() const { return _SFX; }
    PXDevice& getPXDevice() const { return _PFX; }
    /// Get elapsed time since kernel initialization
    inline U64 getCurrentTime() const { return _currentTime; }
    inline U64 getCurrentTimeDelta() const { return _currentTimeDelta; }
    /// Get a pointer to the kernel's threadpool to add,remove,pause or stop
    /// tasks
    inline boost::threadpool::pool* const getThreadPool() {
        assert(_mainTaskPool != nullptr);
        return _mainTaskPool;
    }

    CameraManager& getCameraMgr() { return *_cameraMgr; }

    bool setCursorPosition(U16 x, U16 y) const;
    /// Key pressed
    bool onKeyDown(const Input::KeyEvent& key);
    /// Key released
    bool onKeyUp(const Input::KeyEvent& key);
    /// Joystick axis change
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

     /**
     * @brief Creates a new Task that runs in a separate thread
     * @param tickInterval The delay (in microseconds) between each callback
     * @param numberOfTicks The number of times to call the callback function
     * before the Task is deleted. 0 = run forever
     * @param onCompletionFunction The callback function to call when the thread finishes
     */
    Task* AddTask(
        U64 tickInterval, I32 numberOfTicks,
        const DELEGATE_CBK<>& threadedFunction,
        const DELEGATE_CBK<>& onCompletionFunction = DELEGATE_CBK<>());

   private:
    static void firstLoop();
    void shutdown();
    void renderScene();
    void renderSceneAnaglyph();
    bool mainLoopScene(FrameEvent& evt);
    bool presentToScreen(FrameEvent& evt);
    void threadPoolCompleted(U64 onExitTaskID);

   private:
    void submitRenderCall(RenderStage stage,
                          const SceneRenderState& sceneRenderState,
                          const DELEGATE_CBK<>& sceneRenderCallback) const;

   private:
    Application& _APP;
    /// Access to the GPU
    GFXDevice& _GFX;
    /// Access to the audio device
    SFXDevice& _SFX;
    /// Access to the physics system
    PXDevice& _PFX;
    /// The graphical user interface
    GUI& _GUI;
    /// The input interface
    Input::InputInterface& _input;
    /// The SceneManager/ Scene Pool
    SceneManager& _sceneMgr;
    /// Keep track of all active cameras used by the engine
    std::unique_ptr<CameraManager> _cameraMgr;

    static bool _keepAlive;
    static bool _renderingPaused;
    static bool _freezeLoopTime;
    static bool _freezeGUITime;
    boost::threadpool::pool* _mainTaskPool;
    // both are in ms
    static U64 _currentTime;
    static U64 _currentTimeFrozen;
    static U64 _currentTimeDelta;
    static U64 _previousTime;
    static D32 _nextGameTick;

    static SharedLock _threadedCallbackLock;
    static vectorImpl<U64> _threadedCallbackBuffer;
    static hashMapImpl<U64, DELEGATE_CBK<> > _threadedCallbackFunctions;

    static Util::GraphPlot2D _appTimeGraph;
    // Command line arguments
    I32 _argc;
    char** _argv;
};

namespace Attorney {
class KernelScene {
   private:
    static void submitRenderCall(const Kernel& kernel, RenderStage stage,
                                 const SceneRenderState& sceneRenderState,
                                 const DELEGATE_CBK<>& sceneRenderCallback) {
        kernel.submitRenderCall(stage, sceneRenderState, sceneRenderCallback);
    }

    friend class Divide::SceneManager;
};
};  // namespace Attorney
};  // namespace Divide

#endif  //_CORE_KERNEL_H_
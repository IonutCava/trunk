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

#ifndef _CORE_KERNEL_H_
#define _CORE_KERNEL_H_

#include "TaskPool.h"
#include "Core/Headers/Application.h"
#include "Managers/Headers/CameraManager.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"



namespace Divide {

class Scene;
class PXDevice;
class GFXDevice;
class SFXDevice;
class Application;
class SceneManager;
class InputInterface;
class SceneRenderState;
class FrameListenerManager;

enum class RenderStage : U32;

struct FrameEvent;
class GUI;

struct LoopTimingData {
    LoopTimingData();

    // number of scene update loops
    U8 _updateLoops;

    bool _keepAlive;
    bool _freezeLoopTime;
    // both are in ms
    U64 _currentTime;
    U64 _currentTimeFrozen;
    U64 _currentTimeDelta;
    U64 _previousTime;
    U64 _nextGameTick;
};


namespace Attorney {
    class KernelApplication;
};

namespace Time {
    class ProfileTimer;
};
/// The kernel is the main interface to our engine components:
///-video
///-audio
///-physx
///-scene manager
///-etc
class Kernel : public Input::InputAggregatorInterface, private NonCopyable {
    friend class Attorney::KernelApplication;
   public:
    Kernel(I32 argc, char** argv, Application& parentApp);
    ~Kernel();

    static bool onStartup();
    static bool onShutdown();

    /// Our main application rendering loop.
    /// Call input requests, physics calculations, pre-rendering,
    /// rendering,post-rendering etc
    void onLoop();
    /// Called after a swap-buffer call and before a clear-buffer call.
    /// In a GPU-bound application, the CPU will wait on the GPU to finish
    /// processing the frame
    /// so this should keep it busy (old-GLUT heritage)
    void idle();
    GFXDevice& getGFXDevice() const { return _GFX; }
    SFXDevice& getSFXDevice() const { return _SFX; }
    PXDevice& getPXDevice() const { return _PFX; }
    
    CameraManager& getCameraMgr() { return *_cameraMgr; }

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

    inline TaskPool& taskPool() {
        return _taskPool;
    }

    inline const TaskPool& taskPool() const {
        return _taskPool;
    }

   private:
    ErrorCode initialize(const stringImpl& entryPoint);
    void warmup();
    void shutdown();
    bool mainLoopScene(FrameEvent& evt, const U64 deltaTime);
    bool presentToScreen(FrameEvent& evt, const U64 deltaTime);
    bool setCursorPosition(I32 x, I32 y) const;
    /// Update all engine components that depend on the current screen size
    void onChangeWindowSize(U16 w, U16 h);
    void onChangeRenderResolution(U16 w, U16 h) const;

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
    CameraManager* _cameraMgr;

    LoopTimingData _timingData;

    TaskPool _taskPool;

    Time::ProfileTimer& _appLoopTimer;
    Time::ProfileTimer& _frameTimer;
    Time::ProfileTimer& _appIdleTimer;
    Time::ProfileTimer& _appScenePass;
    Time::ProfileTimer& _physicsUpdateTimer;
    Time::ProfileTimer& _physicsProcessTimer;
    Time::ProfileTimer& _sceneUpdateTimer;
    Time::ProfileTimer& _sceneUpdateLoopTimer;
    Time::ProfileTimer& _cameraMgrTimer;
    Time::ProfileTimer& _flushToScreenTimer;
    // Command line arguments
    I32 _argc;
    char** _argv;
};

namespace Attorney {
    class KernelApplication {
        static ErrorCode initialize(Kernel& kernel, const stringImpl& entryPoint) {
            return kernel.initialize(entryPoint);
        }

        static void shutdown(Kernel& kernel) {
            kernel.shutdown();
        }

        static bool setCursorPosition(Kernel& kernel, I32 x, I32 y) {
            return kernel.setCursorPosition(x, y);
        }

        static void onChangeWindowSize(Kernel& kernel, U16 w, U16 h) {
            kernel.onChangeWindowSize(w, h);
        }

        static void onChangeRenderResolution(Kernel& kernel, U16 w, U16 h) {
            kernel.onChangeRenderResolution(w, h);
        }

        static void warmup(Kernel& kernel) {
            kernel.warmup();
        }

        static void onLoop(Kernel& kernel) {
            kernel.onLoop();
        }

        // threadID = calling thread
        // beginSync = true, called before thread processes data / false, called when thread finished processing data
        static void syncThreadToGPU(Kernel& kernel, const std::thread::id& threadID, bool beginSync);

        friend class Divide::Application;
        friend class Divide::Attorney::ApplicationTask;
    };
};

};  // namespace Divide

#endif  //_CORE_KERNEL_H_
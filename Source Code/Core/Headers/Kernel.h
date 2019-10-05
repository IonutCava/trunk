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
#ifndef _CORE_KERNEL_H_
#define _CORE_KERNEL_H_

#include "EngineTaskPool.h"
#include "Core/Headers/Application.h"
#include "Platform/Input/Headers/InputAggregatorInterface.h"

namespace Divide {

class Scene;
class Editor;
class PXDevice;
class GFXDevice;
class SFXDevice;
class Application;
class SceneManager;
class ResourceCache;
class DebugInterface;
class PlatformContext;
class SceneRenderState;
class RenderPassManager;
class FrameListenerManager;

namespace Input {
    class InputInterface;
};

enum class RenderStage : U8;

struct FrameEvent;

/// Application update rate
constexpr U32 TICKS_PER_SECOND = Config::TARGET_FRAME_RATE / Config::TICK_DIVISOR;

class LoopTimingData {
  public:
    LoopTimingData();

    bool _keepAlive;
  
    inline void update(const U64 elapsedTimeUS) noexcept {
        _previousTimeUS = _currentTimeUS;
        _currentTimeUS = elapsedTimeUS;
        _currentTimeDeltaUS = _currentTimeUS - _previousTimeUS;

        // In case we break in the debugger
        if (_currentTimeDeltaUS > Time::SecondsToMicroseconds(1)) {
            _currentTimeDeltaUS = Time::SecondsToMicroseconds(1) / TICKS_PER_SECOND;
            _previousTimeUS = _currentTimeUS - _currentTimeDeltaUS;
        }
    }

    // return true on change
    inline bool freezeTime(bool state) noexcept {
        if (_freezeLoopTime != state) {
            _freezeLoopTime = state;
            _currentTimeFrozenUS = _currentTimeUS;
            return true;
        }
        return false;
    }

    inline bool runUpdateLoop() noexcept {
        if (_currentTimeUS > _nextGameTickUS && _updateLoops < Config::MAX_FRAMESKIP) {
            return true;
        }

        _updateLoops = 0;
        return false;
    }

    inline void endUpdateLoop(const U64 deltaTimeUS, const bool fixedTimestep) noexcept {
        _nextGameTickUS += deltaTimeUS;
        ++_updateLoops;

        if (fixedTimestep) {
            if (_updateLoops == Config::MAX_FRAMESKIP && _currentTimeUS > _nextGameTickUS) {
                _nextGameTickUS = _currentTimeUS;
            }
        } else {
            _nextGameTickUS = _currentTimeUS;
        }
    }

    inline bool freezeTime() const noexcept {
        return _freezeLoopTime;
    }

    inline U64 currentTimeUS() const noexcept {
        return _currentTimeUS;
    }

    inline U64 currentTimeDeltaUS(bool ignoreFreeze = false) const noexcept {
        return (!ignoreFreeze && _freezeLoopTime) ? 0ULL : _currentTimeDeltaUS;
    }

    inline U64 nextGameTickUS() const noexcept {
        return _nextGameTickUS;
    }

    inline U8 updateLoops() const noexcept {
        return _updateLoops;
    }

  protected:
    // number of scene update loops
    U8 _updateLoops;
    bool _freezeLoopTime;
    U64 _currentTimeUS;
    U64 _currentTimeFrozenUS;
    U64 _currentTimeDeltaUS;
    U64 _previousTimeUS;
    U64 _nextGameTickUS;
};

namespace Attorney {
    class KernelApplication;
    class KernelWindowManager;
    class KernelDebugInterface;
};

namespace Time {
    class ProfileTimer;
};

/// The kernel is the main system that connects all of our various systems: windows, gfx, sfx, input, physics, timing, etc
class Kernel : public Input::InputAggregatorInterface,
               private NonCopyable
{
    friend class Attorney::KernelApplication;
    friend class Attorney::KernelWindowManager;
    friend class Attorney::KernelDebugInterface;

   public:
    Kernel(I32 argc, char** argv, Application& parentApp);
    ~Kernel();

    /// Our main application rendering loop.
    /// Call input requests, physics calculations, pre-rendering,
    /// rendering,post-rendering etc
    void onLoop();
    /// Called after a swap-buffer call and before a clear-buffer call.
    /// In a GPU-bound application, the CPU will wait on the GPU to finish
    /// processing the frame
    /// so this should keep it busy (old-GLUT heritage)
    void idle(bool fast);
    
    /// Key pressed
    bool onKeyDown(const Input::KeyEvent& key) override;
    /// Key released
    bool onKeyUp(const Input::KeyEvent& key) override;
    /// Joystick axis change
    bool joystickAxisMoved(const Input::JoystickEvent& arg) override;
    /// Joystick direction change
    bool joystickPovMoved(const Input::JoystickEvent& arg) override;
    /// Joystick button pressed
    bool joystickButtonPressed(const Input::JoystickEvent& arg) override;
    /// Joystick button released
    bool joystickButtonReleased(const Input::JoystickEvent& arg) override;
    bool joystickBallMoved(const Input::JoystickEvent& arg) override;
    bool joystickAddRemove(const Input::JoystickEvent& arg) override;
    bool joystickRemap(const Input::JoystickEvent &arg) override;
    /// Mouse moved
    bool mouseMoved(const Input::MouseMoveEvent& arg) override;
    /// Mouse button pressed
    bool mouseButtonPressed(const Input::MouseButtonEvent& arg) override;
    /// Mouse button released
    bool mouseButtonReleased(const Input::MouseButtonEvent& arg) override;

    bool onUTF8(const Input::UTF8Event& arg) override;

    ResourceCache& resourceCache() {
        assert(_resCache != nullptr);
        return *_resCache;
    }

    const ResourceCache& resourceCache() const {
        assert(_resCache != nullptr);
        return *_resCache;
    }

    PlatformContext& platformContext() {
        assert(_platformContext != nullptr);
        return *_platformContext;
    }

    const PlatformContext& platformContext() const {
        assert(_platformContext != nullptr);
        return *_platformContext;
    }

    SceneManager& sceneManager() {
        assert(_sceneManager != nullptr);
        return *_sceneManager;
    }

    const SceneManager& sceneManager() const {
        assert(_sceneManager != nullptr);
        return *_sceneManager;
    }

    RenderPassManager& renderPassManager() {
        assert(_renderPassManager != nullptr);
        return *_renderPassManager;
    }

    const RenderPassManager& renderPassManager() const {
        assert(_renderPassManager != nullptr);
        return *_renderPassManager;
    }

   private:
    ErrorCode initialize(const stringImpl& entryPoint);
    void warmup();
    void shutdown();
    void startSplashScreen();
    void stopSplashScreen();
    bool mainLoopScene(FrameEvent& evt, 
                       const U64 deltaTimeUS,     //Framerate independent deltaTime. Can be paused. (e.g. used by scene updates)
                       const U64 realDeltaTimeUS, //Framerate dependent deltaTime. Can be paused. (e.g. used by physics)
                       const U64 appDeltaTimeUS); //Real app delta time between frames. Can't be paused (e.g. used by editor)
    bool presentToScreen(FrameEvent& evt, const U64 deltaTimeUS);
    bool setCursorPosition(I32 x, I32 y);
    /// Update all engine components that depend on the current screen size
    void onSizeChange(const SizeChangeParams& params) const;

   private:

    U8 _prevPlayerCount;
    Rect<I32> _prevViewport;
    vector<Rect<I32>> _editorViewports;
    vector<Rect<I32>> _targetViewports;

    Task* _splashTask = nullptr;
    std::atomic_bool _splashScreenUpdating;

    std::unique_ptr<ResourceCache>     _resCache;
    std::unique_ptr<PlatformContext>   _platformContext;
    std::unique_ptr<SceneManager>      _sceneManager;
    std::unique_ptr<RenderPassManager> _renderPassManager;
    LoopTimingData _timingData;

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
    Time::ProfileTimer& _preRenderTimer;
    Time::ProfileTimer& _postRenderTimer;
    Time::ProfileTimer& _blitToDisplayTimer;
    vector<Time::ProfileTimer*> _renderTimer;

    // Command line arguments
    I32 _argc;
    char** _argv;
};

namespace Attorney {
    class KernelApplication {
      protected:
        static ErrorCode initialize(Kernel& kernel, const stringImpl& entryPoint) {
            return kernel.initialize(entryPoint);
        }

        static void shutdown(Kernel& kernel) {
            kernel.shutdown();
        }

        static void onSizeChange(const Kernel& kernel, const SizeChangeParams& params) {
            kernel.onSizeChange(params);
        }

        static void warmup(Kernel& kernel) {
            kernel.warmup();
        }

        static void onLoop(Kernel& kernel) {
            kernel.onLoop();
        }

        friend class Divide::Application;
        friend class Divide::Attorney::ApplicationTask;
    };

    class KernelWindowManager {
      protected:
        static bool setCursorPosition(Kernel& kernel, I32 x, I32 y) {
            return kernel.setCursorPosition(x, y);
        }

        friend class Divide::WindowManager;
    };

    class KernelDebugInterface {
    protected:
        static const LoopTimingData& timingData(const Kernel& kernel) noexcept {
            return kernel._timingData;
        }

        friend class Divide::DebugInterface;
    };
};

};  // namespace Divide

#endif  //_CORE_KERNEL_H_
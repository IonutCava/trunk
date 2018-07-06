/*
   Copyright (c) 2014 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "core.h"
#include <boost/noncopyable.hpp>
#include "Hardware/Platform/Headers/Task.h"
#include <boost/lockfree/queue.hpp>
#include "Managers/Headers/CameraManager.h"

class GUI;
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

enum RenderStage;

///Input
namespace OIS {
    class KeyEvent;
    class MouseEvent;
    class JoyStickEvent;
    enum MouseButtonID;
}
struct FrameEvent;

///The kernel is the main interface to our engine components:
///-video
///-audio
///-physx
///-scene manager
///-etc
class Kernel : private boost::noncopyable {
public:
    Kernel(I32 argc, char **argv, Application& parentApp);
    ~Kernel();

    I8 initialize(const std::string& entryPoint);
    void shutdown();

    ///This sets the _mainLoopCallback and starts the main loop
    void beginLogicLoop();
    ///Our main loop entry function. The actual callback can be changed at runtime (i.e. pausing rendering in some menus, or pre-rendering a frame offscreen)
    inline  static void mainLoopStatic() {_mainLoopCallback();}
    ///Our main application rendering loop. Call input requests, physics calculations, pre-rendering, rendering,post-rendering etc
            static void mainLoopApp();
    ///Called after a swap-buffer call and before a clear-buffer call.
    ///In a GPU-bound application, the CPU will wait on the GPU to finish processing the frame so this should keep it busy (old-GLUT heritage)
            static void idle();
    ///Update all engine components that depend on the current resolution
            static void updateResolutionCallback(I32 w, I32 h);

public:

    GFXDevice& getGFXDevice() const {return _GFX;}
    SFXDevice& getSFXDevice() const {return _SFX;}
    PXDevice&  getPXDevice()  const {return _PFX;}
    /// get elapsed time since kernel initialization
    inline U64 getCurrentTime()      const {return _currentTime;}
    inline U64 getCurrentTimeDelta() const {return _currentTimeDelta;}
    /// get a pointer to the kernel's threadpool to add,remove,pause or stop tasks
    inline boost::threadpool::pool* const getThreadPool() {assert(_mainTaskPool != nullptr); return _mainTaskPool;}

    CameraManager& getCameraMgr() { return *_cameraMgr; }

public: ///Input
    ///Key pressed
    bool onKeyDown(const OIS::KeyEvent& key);
    ///Key released
    bool onKeyUp(const OIS::KeyEvent& key);
    ///Joystic axis change
    bool onJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis,I32 deadZone);
    ///Joystick direction change
    bool onJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov);
    ///Joystick button pressed
    bool onJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button);
    ///Joystick button released
    bool onJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button);
    bool sliderMoved( const OIS::JoyStickEvent &arg, I8 index);
    bool vector3Moved( const OIS::JoyStickEvent &arg, I8 index);
    ///Mouse moved
    bool onMouseMove(const OIS::MouseEvent& arg);
    ///Mouse button pressed
    bool onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button);
    ///Mouse button released
    bool onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button);

    inline Task* AddTask(D32 tickInterval, bool startOnCreate, I32 numberOfTicks, const DELEGATE_CBK& threadedFunction, const DELEGATE_CBK& onCompletionFunction = DELEGATE_CBK()) {
         Task* taskPtr = New Task(getThreadPool(), tickInterval, startOnCreate, numberOfTicks, threadedFunction);
         taskPtr->connect(DELEGATE_BIND(&Kernel::threadPoolCompleted, this, _1));
         if (!onCompletionFunction.empty()){
             _threadedCallbackFunctions.insert(std::make_pair(taskPtr->getGUID(), onCompletionFunction));
         }
         return taskPtr;
    }

    inline Task* AddTask(D32 tickInterval, bool startOnCreate, bool runOnce, const DELEGATE_CBK& threadedFunction, const DELEGATE_CBK& onCompletionFunction = DELEGATE_CBK()) {
        Task* taskPtr = New Task(getThreadPool(), tickInterval, startOnCreate, runOnce, threadedFunction);
        taskPtr->connect(DELEGATE_BIND(&Kernel::threadPoolCompleted, this, _1));
        if (!onCompletionFunction.empty()){
            _threadedCallbackFunctions.insert(std::make_pair(taskPtr->getGUID(), onCompletionFunction));
        }
        return taskPtr;
    }

private:
   static void firstLoop();
   void renderScene();
   void renderSceneAnaglyph();
   bool mainLoopScene(FrameEvent& evt);
   bool presentToScreen(FrameEvent& evt, const D32 interpolationFactor);
   void threadPoolCompleted(U64 onExitTaskID);

private:
    friend class SceneManager;
    void submitRenderCall(const RenderStage& stage, const SceneRenderState& sceneRenderState, const DELEGATE_CBK& sceneRenderCallback) const;
    
private:
    Application&    _APP;
    ///Access to the GPU
    GFXDevice&		_GFX;
    ///Access to the audio device
    SFXDevice&		_SFX;
    ///Access to the physics system
    PXDevice&		_PFX;
    ///The graphical user interface
    GUI&			_GUI;
    ///The input interface
    InputInterface& _input;
    ///The SceneManager/ Scene Pool
    SceneManager&	_sceneMgr;
    ///Keep track of all active cameras used by the engine
    CameraManager* _cameraMgr;

    static bool   _keepAlive;
    static bool   _renderingPaused;
    static bool   _freezeLoopTime;
    static bool   _freezeGUITime;
   static DELEGATE_CBK      _mainLoopCallback;
   boost::threadpool::pool* _mainTaskPool;
   // both are in ms
   static U64 _currentTime;
   static U64 _currentTimeFrozen;
   static U64 _currentTimeDelta;
   static U64 _previousTime;
   static D32 _nextGameTick;

   static Unordered_map<U64, DELEGATE_CBK > _threadedCallbackFunctions;
   static boost::lockfree::queue<U64, boost::lockfree::capacity<Config::THREAD_LIMIT + 1> >  _callbackBuffer;
   U8 _loops;
   //Command line arguments
   I32    _argc;
   char **_argv;
};

#endif
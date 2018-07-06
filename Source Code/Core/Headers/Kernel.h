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
#include "Managers/Headers/CameraManager.h"
#include "Hardware/Platform/Headers/Task.h"
#include "Hardware/Input/Headers/InputAggregatorInterface.h"

#include <boost/noncopyable.hpp>
#include <boost/lockfree/queue.hpp>

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

struct FrameEvent;

///The kernel is the main interface to our engine components:
///-video
///-audio
///-physx
///-scene manager
///-etc
class Kernel : public InputAggregatorInterface, private boost::noncopyable {
public:
    Kernel(I32 argc, char **argv, Application& parentApp);
    ~Kernel();

    I8 initialize(const std::string& entryPoint);

    void runLogicLoop();
    ///Our main application rendering loop. Call input requests, physics calculations, pre-rendering, rendering,post-rendering etc
            static void mainLoopApp();
    ///Called after a swap-buffer call and before a clear-buffer call.
    ///In a GPU-bound application, the CPU will wait on the GPU to finish processing the frame so this should keep it busy (old-GLUT heritage)
            static void idle();
    ///Update all engine components that depend on the current resolution
            static void updateResolutionCallback(I32 w, I32 h);

    GFXDevice& getGFXDevice() const {return _GFX;}
    SFXDevice& getSFXDevice() const {return _SFX;}
    PXDevice&  getPXDevice()  const {return _PFX;}
    /// get elapsed time since kernel initialization
    inline U64 getCurrentTime()      const {return _currentTime;}
    inline U64 getCurrentTimeDelta() const {return _currentTimeDelta;}
    /// get a pointer to the kernel's threadpool to add,remove,pause or stop tasks
    inline boost::threadpool::pool* const getThreadPool() {assert(_mainTaskPool != nullptr); return _mainTaskPool;}

    CameraManager& getCameraMgr() { return *_cameraMgr; }

    bool setCursorPosition(U16 x, U16 y) const;
    ///Key pressed
    bool onKeyDown(const OIS::KeyEvent& key);
    ///Key released
    bool onKeyUp(const OIS::KeyEvent& key);
    ///Joystick axis change
    bool joystickAxisMoved(const OIS::JoyStickEvent& arg,I8 axis);
    ///Joystick direction change
    bool joystickPovMoved(const OIS::JoyStickEvent& arg,I8 pov);
    ///Joystick button pressed
    bool joystickButtonPressed(const OIS::JoyStickEvent& arg,I8 button);
    ///Joystick button released
    bool joystickButtonReleased(const OIS::JoyStickEvent& arg, I8 button);
    bool joystickSliderMoved( const OIS::JoyStickEvent &arg, I8 index);
    bool joystickVector3DMoved( const OIS::JoyStickEvent &arg, I8 index);
    ///Mouse moved
    bool mouseMoved(const OIS::MouseEvent& arg);
    ///Mouse button pressed
    bool mouseButtonPressed(const OIS::MouseEvent& arg,OIS::MouseButtonID button);
    ///Mouse button released
    bool mouseButtonReleased(const OIS::MouseEvent& arg,OIS::MouseButtonID button);

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
   void shutdown();
   void renderScene();
   void renderSceneAnaglyph();
   bool mainLoopScene(FrameEvent& evt);
   bool presentToScreen(FrameEvent& evt);
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
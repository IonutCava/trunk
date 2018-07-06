/*“Copyright 2009-2013 DIVIDE-Studio”*/
/* This file is part of DIVIDE Framework.

   DIVIDE Framework is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   DIVIDE Framework is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with DIVIDE Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include "core.h"
#include <boost/noncopyable.hpp>
#include "Hardware/Platform/Headers/Thread.h"

class GUI;
class Task;
class Camera;
class PXDevice;
class GFXDevice;
class SFXDevice;
class Application;
class LightManager;
class SceneManager;
class CameraManager;
class ShaderManager;
class FrameListenerManager;
///Input
namespace OIS {
	class KeyEvent;
	class MouseEvent;
	class JoyStickEvent;
	enum MouseButtonID;
}
class InputInterface;
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

	I8 Initialize(const std::string& entryPoint);
	void Shutdown();

	///This sets the _mainLoopCallback and starts the main loop
	void beginLogicLoop();
	///Our main loop entry function. The actual callback can be changed at runtime (i.e. pausing rendering in some menus, or pre-rendering a frame offscreen)
	inline static void MainLoopStatic() {_mainLoopCallback();}
	///Our main application rendering loop. Call input requests, physics calculations, pre-rendering, rendering,post-rendering etc
		   static void MainLoopApp();
	///Called after a swap-buffer call and before a clear-buffer call.
	///In a GPU-bound application, the CPU will wait on the GPU to finish processing the frame so this should keep it busy (old-GLUT heritage)
		   static void Idle();
	///Update all engine components that depend on the current resolution
	static void updateResolutionCallback(I32 w, I32 h);

public:
	GFXDevice& getGFXDevice() const {return _GFX;}
	SFXDevice& getSFXDevice() const {return _SFX;}
	PXDevice&  getPXDevice()  const {return _PFX;}
	/// get elapsed time since kernel initialization
	inline U32 getCurrentTime()   const {return _currentTime;}
	inline U32 getCurrentTimeMS() const {return _currentTimeMS;}
	/// get a pointer to the kernel's threadpool to add,remove,pause or stop tasks
	inline boost::threadpool::pool* const getThreadPool() {assert(_mainTaskPool != NULL); return _mainTaskPool;}

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

private:
   static void FirstLoop();
   bool MainLoopScene();
   bool presentToScreen();

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
	///The SceneManager/ Scene Pool
	SceneManager&	_sceneMgr;
    ///The ShaderMAnager
    ShaderManager&  _shaderMgr;
	///The manager class responsible for sending frame update events
	FrameListenerManager& _frameMgr;
	///Pointer to the current camera
	Camera*			_camera;
	///Access to all of the input devices
	InputInterface& _inputInterface;
	///General light management and rendering (individual lights are handled by each scene)
	///Unloading the lights is a scene level responsibility
	LightManager&   _lightPool;
	static bool   _keepAlive;
	static bool   _applicationReady;
    static bool   _renderingPaused;

private:
   static boost::function0<void> _mainLoopCallback;
   boost::threadpool::pool*      _mainTaskPool;
   static U32     _currentTime;
   static U32     _currentTimeMS;
   CameraManager* _cameraMgr;
   U32 _nextGameTick;
   U8 _loops;
   //Command line arguments
   I32    _argc;
   char **_argv;
};

#endif
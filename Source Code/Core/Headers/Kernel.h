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

class GUI;
class Event;
class Camera;
class PXDevice;
class GFXDevice;
class SFXDevice;
class LightManager;
class SceneManager;
class CameraManager;

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
	Kernel(I32 argc, char **argv);
	~Kernel();

	I8 Initialize(const std::string& entryPoint);
	void Shutdown();
	void beginLogicLoop();

	static void MainLoopApp();
	inline static void MainLoopStatic() {Kernel::_mainLoopCallback();}
	static void Idle();
	///Update all engine components that depend on the current resolution
	static void updateResolutionCallback(I32 w, I32 h);

public: ///Input
	///Key pressed
	bool onKeyDown(const OIS::KeyEvent& key);
	///Key released
	bool onKeyUp(const OIS::KeyEvent& key);
	///Joystic axis change
	bool OnJoystickMoveAxis(const OIS::JoyStickEvent& arg,I8 axis);
	///Joystick direction change
	bool OnJoystickMovePOV(const OIS::JoyStickEvent& arg,I8 pov);
	///Joystick button pressed
	bool OnJoystickButtonDown(const OIS::JoyStickEvent& arg,I8 button);
	///Joystick button released
	bool OnJoystickButtonUp(const OIS::JoyStickEvent& arg, I8 button);
	///Mouse moved
	bool onMouseMove(const OIS::MouseEvent& arg);
	///Mouse button pressed
	bool onMouseClickDown(const OIS::MouseEvent& arg,OIS::MouseButtonID button);
	///Mouse button released
	bool onMouseClickUp(const OIS::MouseEvent& arg,OIS::MouseButtonID button);

	GFXDevice& getGFXDevice() const {return _GFX;}
	SFXDevice& getSFXDevice() const {return _SFX;}
	PXDevice&  getPXDevice()  const {return _PFX;}

private:
   static void FirstLoop();
   bool MainLoopScene();
   void presentToScreen();

private:
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
	///Pointer to the current camera
	Camera*			_camera;
	///Access to all of the input devices
	InputInterface& _inputInterface;
	///General light management and rendering (individual lights are handled by each scene)
	///Unloading the lights is a scene level responsibility
	LightManager&   _lightPool;
	///_aiEvent is the thread handling the AIManager. It is started before each scene's "initializeAI" is called
	///It is destroyed after each scene's "deinitializeAI" is called
	std::tr1::shared_ptr<Event>  _aiEvent;
	static bool   _keepAlive;
	static bool   _applicationReady;
	/// get elapsed time since kernel initialization
	inline static D32 getCurrentTime()   {return _currentTime;}
	inline static D32 getCurrentTimeMS() {return _currentTimeMS;}

private:
   static boost::function0<void> _mainLoopCallback;
   static D32     _currentTime;
   static D32     _currentTimeMS;
   CameraManager* _cameraMgr;
   bool           _loadAI;
   U32 _nextGameTick;
   U8 _loops;
   //Command line arguments
   I32    _argc;
   char **_argv;
};

#endif
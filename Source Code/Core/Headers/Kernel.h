/*“Copyright 2009-2012 DIVIDE-Studio”*/
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
class Camera;
class PXDevice;
class GFXDevice;
class SFXDevice;
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
	Kernel();
	~Kernel();

	I8 Initialize(const std::string& entryPoint);
	void Shutdown();
	void beginLogicLoop();

	static void MainLoopStatic();
	static void Idle();
	///Cache application data; 
	///It's an ugly hack, but it reduces singleton dependencies and gives a slight speed boost
	void refreshAppData();
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

private:
   bool MainLoop();

private:
	GFXDevice&		_GFX;
	SFXDevice&		_SFX;
	PXDevice&		_PFX;
	GUI&			_GUI;
	SceneManager&	_sceneMgr;
	Camera*			_camera;
	InputInterface& _inputInterface;

	static bool   _keepAlive;
	/// get elapsed time since kernel initialization
	inline static D32 getCurrentTime() {return _currentTime;}

private:
   static D32    _currentTime;
   I32           _targetFrameRate;
   CameraManager* _cameraMgr;
};

#endif
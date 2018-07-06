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

private:
   bool MainLoop();

private:
	GFXDevice&    _GFX;
	SFXDevice&    _SFX;
	PXDevice&     _PFX;
	GUI&          _GUI;
	SceneManager& _sceneMgr;
	Camera*       _camera;
	
	static bool   _keepAlive;
	/// get elapsed time since kernel initialization
	inline static D32 getCurrentTime() {return _currentTime;}

private:
   static D32    _currentTime;
   I32           _targetFrameRate;
   CameraManager* _cameraMgr;
};

#endif
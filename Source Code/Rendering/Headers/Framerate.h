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

#ifndef _FRAMERATE_H_
#define _FRAMERATE_H_

#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Core/Headers/Singleton.h"

#if defined( __WIN32__ ) || defined( _WIN32 )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( __APPLE_CC__ ) // Apple OS X
///??
#else //Linux
#include <sys/time.h> 
#endif

//Code from http://www.gamedev.net/reference/articles/article1382.asp
//Copyright: "Frame Rate Independent Movement" by Ben Dilts

DEFINE_SINGLETON(Framerate)

#if defined( __WIN32__ ) || defined( _WIN32 )
  typedef LARGE_INTEGER LI;
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else //Linux
  typedef timeval LI;
#endif

private:
	Framerate() : 
		_count(0),
		_averageFps(0),
		_maxFps(1.175494351e-38F),
		_minFps(3.402823466e+38F),
		_targetFps(60),
		_speedfactor(1),
		_init(false){}
  F32           _targetFps;
  F32           _fps;
  F32           _speedfactor;
  F32           _averageFps,_maxFps,_minFps;
  I16		    _count;
  U32           _elapsedTime;
  LI			_tickspersecond;
  LI			_currentticks;
  LI			_framedelay;
  LI			_startupTime;
  bool          _init;	
  

public:
  void          Init(F32 tfps);
  void          SetSpeedFactor();
  inline F32    getFps(){return _fps;}
  inline F32    getSpeedfactor(){ReadLock r_lock(_speedLockMutex); return _speedfactor;}
  inline F32    getElapsedTime(){if(!_init) return 0.0f; QueryPerformanceCounter(&_currentticks); return (F32)(_currentticks.QuadPart-_startupTime.QuadPart) *1000/(F32)_tickspersecond.QuadPart;}
  void          benchmark();
  mutable Lock  _speedLockMutex;
END_SINGLETON

#endif

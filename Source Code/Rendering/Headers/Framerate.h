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
		_maxFps(std::numeric_limits<F32>::min()),
		_minFps(std::numeric_limits<F32>::max()),
		_targetFrameRate(60),
		_ticksPerMillisecond(0),
		_speedfactor(1),
		_init(false){}

  F32           _targetFrameRate;
  F32           _fps,_averageFps;
  F32           _frameTime;
  F32           _speedfactor;
  F32           _maxFps,_minFps;
  I16		    _count;
  U32           _elapsedTime;
  D32           _ticksPerMillisecond;
  LI			_ticksPerSecond; //Processor's ticks per second
  LI			_currentTicks;   //Current number of ticks
  LI			_frameDelay;     //Previous frame's number of ticks
  LI			_startupTicks;   //Ticks at class initialization
  bool          _init;	
  mutable SharedLock _speedLockMutex;
  mutable SharedLock _fpsLockMutex;
 public:
  void          Init(U8 tfps);
  void          SetSpeedFactor();
  inline F32    getFps(){ReadLock r_lock(_fpsLockMutex); return _fps;}
  inline F32    getFrameTime() {ReadLock r_lock(_fpsLockMutex); return _frameTime;}
  inline F32    getSpeedfactor(){ReadLock r_lock(_speedLockMutex); return _speedfactor;}

  inline U32    getElapsedTime(){ //in milliseconds
	  if(!_init) return 0; 
	  WriteLock w_lock(_speedLockMutex);
	  QueryPerformanceCounter(&_currentTicks); 
	  w_lock.unlock();
	  return static_cast<U32>((_currentTicks.QuadPart-_startupTicks.QuadPart) / _ticksPerMillisecond);
  }

  void          benchmark();

END_SINGLETON

#define FRAME_SPEED_FACTOR  Framerate::getInstance().getSpeedfactor()
#endif

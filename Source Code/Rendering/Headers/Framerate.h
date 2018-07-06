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
#include <boost/atomic.hpp>

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
		_targetFrameRate(60),
		_ticksPerMillisecond(0),
		_speedfactor(1.0f),
		_init(false),
		_benchmark(false){}

  F32           _targetFrameRate;
  F32           _frameTime;
  U32           _elapsedTime;
  LI			_ticksPerSecond; //Processor's ticks per second
  LI			_frameDelay;     //Previous frame's number of ticks
  LI			_startupTicks;   //Ticks at class initialization
  bool          _benchmark;      //Measure average FPS and output max/min/average fps to console

  boost::atomic_bool _init;
  boost::atomic<LI>	 _currentTicks;   //Current number of ticks
  boost::atomic<F32> _speedfactor;
  boost::atomic<F32> _fps;
  boost::atomic<D32> _ticksPerMillisecond;

 public:

  void          Init(U8 tfps);
  void          SetSpeedFactor();

  inline void benchmark(bool state) {_benchmark = state;}

  inline bool benchmark()      const {return _benchmark;}
  inline F32  getFps()         const {return _fps;}
  inline F32  getFrameTime()   const {return _frameTime;}
  inline F32  getSpeedfactor() const {return _speedfactor;}

  inline U32 getElapsedTime(){ //in milliseconds
	  if(!_init) return 0;
	  LI currentTicks;
	  QueryPerformanceCounter(&currentTicks);
	  _currentTicks = currentTicks;
	  return static_cast<U32>((currentTicks.QuadPart-_startupTicks.QuadPart) / _ticksPerMillisecond);
  }

protected:
  void benchmarkInternal();

END_SINGLETON

#define FRAME_SPEED_FACTOR  Framerate::getInstance().getSpeedfactor()
#endif

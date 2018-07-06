#ifndef _FRAMERATE_H_
#define _FRAMERATE_H_

#include "Hardware/Platform/PlatformDefines.h"
#include "Utility/Headers/Singleton.h"

#if defined( __WIN32__ ) || defined( _WIN32 )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( __APPLE_CC__ ) // Apple OS X
//??
#else //Linux
#include <sys/time.h> 
#endif

//Code from http://www.gamedev.net/reference/articles/article1382.asp
//Copyright: "Frame Rate Independent Movement" by Ben Dilts

SINGLETON_BEGIN(Framerate)

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
		_maxFps(0),
		_minFps(60),
		_targetFps(60) {}
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
	
  

public:
  void          Init(F32 tfps);
  void          SetSpeedFactor();
  F32           getFps(){return _fps;}
  F32           getSpeedfactor(){return _speedfactor;}
  F32           getElapsedTime(){QueryPerformanceCounter(&_currentticks); return (F32)(_currentticks.QuadPart-_startupTime.QuadPart) *1000/(F32)_tickspersecond.QuadPart;}
  void          benchmark();

SINGLETON_END()

#endif

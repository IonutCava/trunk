#ifndef _FRAMERATE_H_
#define _FRAMERATE_H_

#include "Utility/Headers/DataTypes.h"
#include "Utility/Headers/Singleton.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

//Code from http://www.gamedev.net/reference/articles/article1382.asp
//Copyright: "Frame Rate Independent Movement" by Ben Dilts

SINGLETON_BEGIN(Framerate)

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
  int		    _count;
  LARGE_INTEGER _tickspersecond;
  LARGE_INTEGER _currentticks;
  LARGE_INTEGER _framedelay;
	
  

public:
  void          Init(F32 tfps);
  void          SetSpeedFactor();
  F32           getFps(){return _fps;}
  F32           getSpeedfactor(){return _speedfactor;}
  void          benchmark();

SINGLETON_END()

#endif

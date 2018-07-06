#include "Framerate.h"
#include <iostream>
#include "Utility/Headers/Console.h"

void Framerate::Init(F32 tfps)
{
  _targetFps = tfps;
   
#if defined( __WIN32__ ) || defined( _WIN32 )
  QueryPerformanceCounter(&_framedelay);
  _startupTime = _framedelay;
  QueryPerformanceFrequency(&_tickspersecond);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else //Linux
  gettimeofday(&_currentticks,NULL);
#endif

}

void Framerate::SetSpeedFactor()
{

#if defined( __WIN32__ ) || defined( _WIN32 )
	 QueryPerformanceCounter(&_currentticks);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else
	gettimeofday(&_currentticks,NULL);
#endif


  _speedfactor = (F32)(_currentticks.QuadPart-_framedelay.QuadPart)/
	            ((F32)_tickspersecond.QuadPart/_targetFps);

  _fps = _targetFps/_speedfactor;
  
  if (_speedfactor <= 0)
    _speedfactor = 1;

  _framedelay = _currentticks;
  /*if(false)*/ benchmark();
}

void Framerate::benchmark()
{

	_averageFps += _fps;

	//Min/Max FPS
	if(_count > 50)
	{
		if(_fps > _maxFps) _maxFps = _fps;
		if(_fps < _minFps) _minFps = _fps;
	}
	
	//Average FPS
	if(_count > 1000)
	{
		_averageFps /= _count;
		 Console::getInstance().printfn("Average FPS: %0.2f; Max FPS: %0.2f; Min FPS: %0.2f" , _averageFps,_maxFps,_minFps); 
		_count = 0;
	}
	++_count;
}
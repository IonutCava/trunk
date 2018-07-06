#include "Headers/Framerate.h"
#include "Core/Headers/Console.h"

void Framerate::Init(U8 tfps){
  _targetFrameRate = tfps;
   
#if defined( __WIN32__ ) || defined( _WIN32 )
  QueryPerformanceCounter(&_framedelay);
  _startupTime = _framedelay;
  QueryPerformanceFrequency(&_tickspersecond);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else //Linux
  gettimeofday(&_currentticks,NULL);
#endif
 _init = true;
}

void Framerate::SetSpeedFactor(){
	WriteLock w_lock(_speedLockMutex); 
#if defined( __WIN32__ ) || defined( _WIN32 )
	 QueryPerformanceCounter(&_currentticks);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else
	gettimeofday(&_currentticks,NULL);
#endif


  _speedfactor = (F32)(_currentticks.QuadPart-_framedelay.QuadPart)/
	            ((F32)_tickspersecond.QuadPart/_targetFrameRate);

  _fps = _targetFrameRate/_speedfactor;
  
  if (_speedfactor <= 0)
    _speedfactor = 1;

  _framedelay = _currentticks;
  /*if(false)*/ benchmark();
}

void Framerate::benchmark(){

	_averageFps += _fps;

	//Min/Max FPS (after ~50 rendered frames)
	if(_count > 50){
		_maxFps = Util::max(_maxFps, _fps);
		_minFps = Util::min(_minFps, _fps);
	}
	
	//Average FPS
	if(_count > 500){
		_averageFps /= _count;
		PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), _averageFps,_maxFps,_minFps,1000.0f/_averageFps); 
		_count = 0;
	}
	++_count;
}
#include "Headers/Framerate.h"
#include "Core/Headers/Console.h"

///No need for init to be threadsafe
void Framerate::Init(U8 tfps){
   assert(!_init);//<prevent double init

  _targetFrameRate = static_cast<F32>(tfps);
   
#if defined( __WIN32__ ) || defined( _WIN32 )
  QueryPerformanceCounter(&_startupTicks);
  if(!QueryPerformanceFrequency(&_ticksPerSecond)){
	  assert("Current system does not support 'QueryPerformanceFrequency calls!");
  };
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else //Linux
  gettimeofday(&_startupTicks,NULL);
#endif
  _ticksPerMillisecond = static_cast<D32>(_ticksPerSecond.QuadPart) / 1000.0;
  _frameDelay = _startupTicks;
  _init = true;
}

void Framerate::SetSpeedFactor(){
	WriteLock w_lock(_speedLockMutex); 
#if defined( __WIN32__ ) || defined( _WIN32 )
	 QueryPerformanceCounter(&_currentTicks);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else
	gettimeofday(&_currentTicks,NULL);
#endif
  _speedfactor = static_cast<F32>((_currentTicks.QuadPart-_frameDelay.QuadPart)/ (static_cast<F32>(_ticksPerSecond.QuadPart) / _targetFrameRate));
//  CLAMP<F32>(_speedfactor,getMsToSec(1),1);
  if(_speedfactor <= 0) _speedfactor = 1;
  _frameDelay = _currentTicks;
  w_lock.unlock();
  
  WriteLock w_fps_lock(_fpsLockMutex);
  ReadLock r_lock(_speedLockMutex);
  _fps = _targetFrameRate/_speedfactor;
  r_lock.unlock();
  _frameTime = 1000.0f/ _fps;
  w_fps_lock.unlock();
  /*if(false)*/ benchmark();
}

void Framerate::benchmark(){
	ReadLock r_fps_lock(_fpsLockMutex);
	F32 fps = _fps;
	r_fps_lock.unlock();

	_averageFps += fps;

	//Min/Max FPS (after ~50 rendered frames)
	if(_count > 50){
		_maxFps = Util::max(_maxFps, fps);
		_minFps = Util::min(_minFps, fps);
	}
	//Average FPS
	if(_count > 500){
		_averageFps /= _count;
		PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), _averageFps,_maxFps,_minFps, 1000.0f / _averageFps); 
		_count = 0;
	}
	++_count;
}
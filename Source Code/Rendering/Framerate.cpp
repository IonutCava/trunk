#include "Headers/Framerate.h"
#include "Core/Headers/Console.h"
#include "Core/Math/Headers/MathHelper.h"
#include "Utility/Headers/Localization.h"

///No need for init to be threadsafe
void Framerate::Init(U8 tfps) {
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
	LI currentTicks;

#if defined( __WIN32__ ) || defined( _WIN32 )
	 QueryPerformanceCounter(&currentTicks);
	 QueryPerformanceFrequency(&_ticksPerSecond);
#elif defined( __APPLE_CC__ ) // Apple OS X
	//??
#else
	gettimeofday(&currentTicks,NULL);
#endif
  _ticksPerMillisecond = static_cast<D32>(_ticksPerSecond.QuadPart) / 1000.0;
  _speedfactor = static_cast<F32>((currentTicks.QuadPart-_frameDelay.QuadPart)/ (static_cast<F32>(_ticksPerSecond.QuadPart) / _targetFrameRate));
//  CLAMP<F32>(_speedfactor,getMsToSec(1),1);
  if(_speedfactor <= 0) _speedfactor = 1;
  _frameDelay = currentTicks;

  _fps = _targetFrameRate/_speedfactor;
  _frameTime = 1000.0f/ _fps;
  _currentTicks = currentTicks;
  if(_benchmark) benchmarkInternal();
}

void Framerate::benchmarkInternal(){

	static U32 count = 0;
	static F32 maxFps = std::numeric_limits<F32>::min();
	static F32 minFps = std::numeric_limits<F32>::max();
	static F32 averageFps = 0.0f;
	static bool resetAverage = false;

	F32 fps = _fps;
	//Average FPS
	averageFps += fps;

	//Min/Max FPS (after every target second)
	if(count > _targetFrameRate){
		maxFps = Util::max(maxFps, fps);
		minFps = Util::min(minFps, fps);
	}
	//Every 10 seconds (targeted)
	if(count > _targetFrameRate * 10){
		averageFps /= count;
		PRINT_FN(Locale::get("FRAMERATE_FPS_OUTPUT"), averageFps, maxFps, minFps, 1000.0f / averageFps);
		count = 0;
		if(resetAverage) averageFps = 0.0f;
		resetAverage = !resetAverage;
	}
	++count;
}
#include "Framerate.h"
#include <iostream>

void Framerate::Init(F32 tfps)
{
  _targetFps = tfps;
  QueryPerformanceCounter(&_framedelay);
  QueryPerformanceFrequency(&_tickspersecond);
}

void Framerate::SetSpeedFactor()
{
  QueryPerformanceCounter(&_currentticks);
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
		 cout << "Average FPS: " << _averageFps << "; Max FPS: " <<  _maxFps 
							<< "; Min FPS: " << _minFps << endl;
		_count = 0;
	}
	++_count;
}
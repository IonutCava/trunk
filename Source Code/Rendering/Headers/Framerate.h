/*
   Copyright (c) 2013 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy of this software
   and associated documentation files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _FRAMERATE_H_
#define _FRAMERATE_H_

#include "config.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"
#include "Core/Headers/Singleton.h"
#include <boost/atomic.hpp>

#if defined( OS_WINDOWS )
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined( OS_APPLE ) // Apple OS X
///??
#else //Linux
#include <sys/time.h>
#endif

//Code from http://www.gamedev.net/reference/articles/article1382.asp
//Copyright: "Frame Rate Independent Movement" by Ben Dilts

DEFINE_SINGLETON(Framerate)

#if defined( OS_WINDOWS )
  typedef LARGE_INTEGER LI;
#elif defined( OS_APPLE ) // Apple OS X
    //??
#else //Linux
  typedef timeval LI;
#endif

private:
    Framerate() : _targetFrameRate(60),
                  _ticksPerMillisecond(0),
                  _speedfactor(1.0f),
                  _init(false),
                  _benchmark(false)
    {
    }

    U8   _targetFrameRate;
    F32  _frameTime;
    LI	 _ticksPerSecond; //Processor's ticks per second
    LI	 _frameDelay;     //Previous frame's number of ticks
    LI	 _startupTicks;   //Ticks at class initialization
    LI   _currentTicks;
    bool _benchmark;      //Measure average FPS and output max/min/average fps to console

    boost::atomic_bool _init;
    boost::atomic_uint _ticksPerMillisecond;
    boost::atomic<F32> _speedfactor;
    boost::atomic<F32> _fps;
public:

    void Init(U8 targetFrameRate);
    void SetSpeedFactor();

    inline void benchmark(bool state)  {_benchmark = state;}
    inline bool benchmark()      const {return _benchmark;}
    inline F32  getFps()         const {return _fps;}
    inline F32  getFrameTime()   const {return _frameTime;}
    inline F32  getSpeedfactor() const {return _speedfactor;}

    inline U32 getElapsedTime(){ //in milliseconds
        if(!_init)
            return 0;

        QueryPerformanceCounter(&_currentTicks);
        return (_currentTicks.QuadPart-_startupTicks.QuadPart) / _ticksPerMillisecond;
  }

protected:
  void benchmarkInternal();

END_SINGLETON

#define FRAME_SPEED_FACTOR  Framerate::getInstance().getSpeedfactor()
#endif

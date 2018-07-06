/*
   Copyright (c) 2014 DIVIDE-Studio
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

#ifndef _APPLICATION_TIMER_H_
#define _APPLICATION_TIMER_H_

#include "config.h"
#include "Hardware/Platform/Headers/PlatformDefines.h"

#if defined ( OS_WINDOWS )
    #include <windows.h>
#endif

//Code from http://www.gamedev.net/reference/articles/article1382.asp
//Copyright: "Frame Rate Independent Movement" by Ben Dilts

namespace Divide {

DEFINE_SINGLETON(ApplicationTimer)

#if defined( OS_WINDOWS )
  typedef LARGE_INTEGER LI;
#else
  typedef timeval LI;
#endif

private:
    ApplicationTimer();

    F32  _fps;
    F32  _frameTime;
    F32  _speedfactor;
    U32  _targetFrameRate; 
    D32  _ticksPerMicrosecond;
    LI	 _ticksPerSecond; //Processor's ticks per second
    LI	 _frameDelay;     //Previous frame's number of ticks
    LI	 _startupTicks;   //Ticks at class initialization
    LI   _currentTicks;
    bool _benchmark;      //Measure average FPS and output max/min/average fps to console
    bool _init;
    
    std::atomic<U64> _elapsedTimeUs;

public:

    void init(U8 targetFrameRate);
    void update(U32 frameCount);

    inline void benchmark(bool state)  {_benchmark = state;}
    inline bool benchmark()      const {return _benchmark;}
    inline F32  getFps()         const {return _fps;}
    inline F32  getFrameTime()   const {return _frameTime;}
    inline F32  getSpeedfactor() const {return _speedfactor;}

    inline U64 getElapsedTime(bool state = false) { 
        if(state) return getElapsedTimeInternal();
        else      return _elapsedTimeUs; 
    }

protected:
  void benchmarkInternal(U32 frameCount);
  U64  getElapsedTimeInternal();

#if defined(_DEBUG) || defined(_PROFILE)
  friend class ProfileTimer;
  void addTimer(ProfileTimer* const timer);
  void removeTimer(ProfileTimer* const timer);
  vectorImpl<ProfileTimer* > _profileTimers;
#endif

END_SINGLETON

#define FRAME_SPEED_FACTOR  ApplicationTimer::getInstance().getSpeedfactor()

#if defined(_DEBUG) || defined(_PROFILE)
class ProfileTimer {
public:
    ProfileTimer();
    ~ProfileTimer();

    void create(const char* name);
    void start();
    void stop();
    void print() const;
    void reset();

    inline const char* name() const {return _name;}
    inline D32         get()  const {return _timer;}
    inline bool        init() const {return _init;}
    inline void pause(const bool state) {_paused = state;}

protected:
    const char*        _name;
    std::atomic_bool _paused;
	std::atomic_bool _init;
	std::atomic<D32> _timer;
	std::atomic<D32> _timerAverage;
	std::atomic_int  _timerCounter;
};

    inline ProfileTimer* ADD_TIMER(const char* timerName) {
        ProfileTimer* timer = New ProfileTimer();
        timer->create(timerName); 
        return timer;
    }

    inline void START_TIMER(ProfileTimer* const timer)  {
        assert(timer);
        timer->start();
    }

    inline void STOP_TIMER(ProfileTimer* const timer)   {
        assert(timer);
        timer->stop();
    }

    inline void PRINT_TIMER(ProfileTimer* const timer)  {
        assert(timer);
        timer->print();
    }

#else
    class ProfileTimer { 
        public:
            inline void pause(const bool state) {}
    };
    inline ProfileTimer* ADD_TIMER(const char* timerName) {return nullptr;}
    inline void START_TIMER(ProfileTimer* const timer)  {}
    inline void STOP_TIMER(ProfileTimer* const timer)   {}
    inline void PRINT_TIMER(ProfileTimer* const timer)  {}
#endif //_DEBUG

    inline void REMOVE_TIMER(ProfileTimer*& timer) { MemoryManager::SAFE_DELETE(timer);  }

}; //namespace Divide

#endif //_FRAMERATE_H_

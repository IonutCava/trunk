/*
   Copyright (c) 2015 DIVIDE-Studio
   Copyright (c) 2009 Ionut Cava

   This file is part of DIVIDE Framework.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software
   and associated documentation files (the "Software"), to deal in the Software
   without restriction,
   including without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY,
   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
   IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#ifndef _CORE_APPLICATION_TIMER_H_
#define _CORE_APPLICATION_TIMER_H_

#include "Core/Math/Headers/MathHelper.h"

// Code from http://www.gamedev.net/reference/articles/article1382.asp
// Copyright: "Frame Rate Independent Movement" by Ben Dilts

namespace Divide {
namespace Time {
class ProfileTimer;
DEFINE_SINGLETON(ApplicationTimer)
  public:
    void init(U8 targetFrameRate);
    void update(U32 frameCount);

    void benchmark(bool state);
    bool benchmark() const;
    F32 getFps() const;
    F32 getFrameTime() const;
    F32 getSpeedfactor() const;
    U64 getElapsedTime(bool forceUpdate = false);

  protected:
    ApplicationTimer();
    ~ApplicationTimer();

    void benchmarkInternal(U32 frameCount);
    U64 getElapsedTimeInternal(TimeValue currentTicks) const;
    TimeValue getCurrentTicksInternal() const;
    U64 getElapsedTimeInternal() const;

    friend class ProfileTimer;
    void addTimer(ProfileTimer* const timer);
    void removeTimer(ProfileTimer* const timer);
    vectorImpl<ProfileTimer*> _profileTimers;

  private:
    F32 _fps;
    F32 _frameTime;
    F32 _speedfactor;
    U32 _targetFrameRate;
    TimeValue _ticksPerSecond;  // Processor's ticks per second
    TimeValue _frameDelay;      // Previous frame's number of ticks
    TimeValue _startupTicks;    // Ticks at class initialization
    bool _benchmark;            // Measure average FPS and output max/min/average fps to console
    bool _init;

    std::atomic<U64> _elapsedTimeUs;
END_SINGLETON

/// The following functions force a timer update 
/// (a call to query performance timer).
inline U64 ElapsedMicroseconds(bool forceUpdate = false) {
    return ApplicationTimer::getInstance().getElapsedTime(forceUpdate);
}

inline D32 ElapsedMilliseconds(bool forceUpdate = false) {
    return MicrosecondsToMilliseconds<D32>(ElapsedMicroseconds(forceUpdate));
}
inline D32 ElapsedSeconds(bool forceUpdate = false) {
    return MicrosecondsToSeconds<D32>(ElapsedMicroseconds(forceUpdate));
}

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_APPLICATION_TIMER_H_

#include "ApplicationTimer.inl"

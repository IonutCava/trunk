/*
   Copyright (c) 2018 DIVIDE-Studio
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

#pragma once
#ifndef _CORE_TIME_APPLICATION_TIMER_H_
#define _CORE_TIME_APPLICATION_TIMER_H_

#include "FrameRateHandler.h"

// Code from http://www.gamedev.net/reference/articles/article1382.asp
// Copyright: "Frame Rate Independent Movement" by Ben Dilts
namespace Divide {
namespace Time {
  
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimeValue;

DEFINE_SINGLETON(ApplicationTimer)
    typedef std::chrono::nanoseconds  NSec;
    typedef std::chrono::microseconds USec;
    typedef std::chrono::milliseconds MSec;
    typedef std::chrono::seconds Sec;

  public:
    void update();

    void resetFPSCounter();
    F32 getFps() const;
    F32 getFrameTime() const;
    F32 getSpeedfactor() const;

    const stringImpl& benchmarkReport() const;

    // Returns the elapsed time since app startup in microseconds
    U64 getElapsedTime(bool forceUpdate = false) const;

  protected:
    ApplicationTimer() noexcept;
    ~ApplicationTimer() = default;
    
    TimeValue getCurrentTicksInternal() const;
    
    U64 getElapsedTimeInternal(const TimeValue& currentTicks) const;

  private:
    FrameRateHandler _frameRateHandler;
    F32 _speedfactor;
    U32 _targetFrameRate;
    // Previous frame's time stamp
    TimeValue _frameDelay;
    // Time stamp at class initialization
    TimeValue _startupTicks;
    // Measure average FPS and output max/min/average fps to console
    U64  _lastBenchmarkTimeStamp;
    std::atomic<U64> _elapsedTimeUs;

    stringImpl _lastBenchmarkReport;
END_SINGLETON

/// The following functions force a timer update 
/// (a call to query performance timer).
inline U64 ElapsedNanoseconds(bool forceUpdate = false);
inline U64 ElapsedMicroseconds(bool forceUpdate = false);

inline D64 ElapsedMilliseconds(bool forceUpdate = false);
inline D64 ElapsedSeconds(bool forceUpdate = false);

};  // namespace Time
};  // namespace Divide

#endif  //_CORE_TIME_APPLICATION_TIMER_H_

#include "ApplicationTimer.inl"
